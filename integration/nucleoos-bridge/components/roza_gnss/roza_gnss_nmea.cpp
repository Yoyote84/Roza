#include "roza_gnss_nmea.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

static void copy_field(char *dst, size_t capacity, const char *src)
{
    if (!dst || capacity == 0) return;
    if (!src) src = "";
    snprintf(dst, capacity, "%s", src);
}

static bool parse_number(const char *text, float *value)
{
    if (!text || !*text || !value) return false;
    char *end = nullptr;
    const float parsed = strtof(text, &end);
    if (end == text || *end != '\0') return false;
    *value = parsed;
    return true;
}

static bool parse_coordinate(const char *raw, const char *hemisphere, double *value)
{
    if (!raw || !*raw || !hemisphere || !*hemisphere || !value) return false;
    char *end = nullptr;
    const double packed = strtod(raw, &end);
    if (end == raw || *end != '\0' || packed < 0.0) return false;

    const int degrees = static_cast<int>(packed / 100.0);
    const double minutes = packed - static_cast<double>(degrees) * 100.0;
    if (minutes < 0.0 || minutes >= 60.0) return false;

    double decimal = static_cast<double>(degrees) + minutes / 60.0;
    const char direction = static_cast<char>(toupper(static_cast<unsigned char>(hemisphere[0])));
    if (direction == 'S' || direction == 'W') decimal = -decimal;
    else if (direction != 'N' && direction != 'E') return false;
    *value = decimal;
    return true;
}

static void split_fields(char *line, char **fields, int capacity, int *count)
{
    *count = 0;
    if (!line || !fields || capacity <= 0) return;
    fields[(*count)++] = line;
    for (char *p = line; *p && *count < capacity; ++p) {
        if (*p == ',') {
            *p = '\0';
            fields[(*count)++] = p + 1;
        }
    }
}

} // namespace

extern "C" void roza_gnss_fix_reset(roza_gnss_fix_t *fix)
{
    if (fix) memset(fix, 0, sizeof(*fix));
}

extern "C" bool roza_gnss_parse_sentence(const char *sentence, roza_gnss_fix_t *fix)
{
    if (!sentence || !fix || sentence[0] != '$') return false;

    char line[160];
    const size_t length = strlen(sentence);
    if (length < 8 || length >= sizeof(line)) return false;
    memcpy(line, sentence, length + 1);

    while (length > 0 && (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n')) {
        line[strlen(line) - 1] = '\0';
    }

    char *star = strchr(line, '*');
    if (!star || strlen(star + 1) < 2) return false;

    unsigned int expected = 0;
    if (sscanf(star + 1, "%2x", &expected) != 1) return false;

    unsigned char checksum = 0;
    for (const char *p = line + 1; p < star; ++p) checksum ^= static_cast<unsigned char>(*p);
    if (checksum != static_cast<unsigned char>(expected)) return false;
    *star = '\0';

    char *fields[20] = {};
    int field_count = 0;
    split_fields(line, fields, 20, &field_count);
    if (field_count < 2 || strlen(fields[0]) < 6) return false;

    const char *kind = fields[0] + 3; // $GPxxx, $GNxxx, $GLxxx, ...
    if (strcmp(kind, "GGA") == 0) {
        if (field_count < 10) return false;

        const int quality = atoi(fields[6]);
        fix->has_fix = quality > 0;
        fix->satellites = atoi(fields[7]);

        float number = 0.0f;
        if (parse_number(fields[8], &number)) fix->hdop = number;
        if (parse_number(fields[9], &number)) fix->altitude_m = number;
        copy_field(fix->utc_time, sizeof(fix->utc_time), fields[1]);

        double coordinate = 0.0;
        if (fix->has_fix &&
            parse_coordinate(fields[2], fields[3], &coordinate)) {
            fix->latitude_deg = coordinate;
        }
        if (fix->has_fix &&
            parse_coordinate(fields[4], fields[5], &coordinate)) {
            fix->longitude_deg = coordinate;
        }
        return true;
    }

    if (strcmp(kind, "RMC") == 0) {
        if (field_count < 10) return false;
        fix->has_fix = fields[2][0] == 'A';

        double coordinate = 0.0;
        if (fix->has_fix &&
            parse_coordinate(fields[3], fields[4], &coordinate)) {
            fix->latitude_deg = coordinate;
        }
        if (fix->has_fix &&
            parse_coordinate(fields[5], fields[6], &coordinate)) {
            fix->longitude_deg = coordinate;
        }

        float number = 0.0f;
        if (parse_number(fields[7], &number)) fix->speed_kmh = number * 1.852f;
        if (parse_number(fields[8], &number)) fix->course_deg = number;
        copy_field(fix->utc_time, sizeof(fix->utc_time), fields[1]);
        copy_field(fix->utc_date, sizeof(fix->utc_date), fields[9]);
        return true;
    }

    return false;
}
