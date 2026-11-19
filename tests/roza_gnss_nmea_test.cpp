#include "roza_gnss_nmea.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

static bool near(double actual, double expected, double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}

static std::string make_nmea_sentence(const char *payload)
{
    unsigned char checksum = 0;
    for (const char *p = payload; *p; ++p) checksum ^= static_cast<unsigned char>(*p);
    char suffix[8];
    snprintf(suffix, sizeof(suffix), "*%02X", checksum);
    return std::string("$") + payload + suffix;
}

int main()
{
    roza_gnss_fix_t fix = {};
    const char *gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
    assert(roza_gnss_parse_sentence(gga, &fix));
    assert(fix.has_fix);
    assert(fix.satellites == 8);
    assert(near(fix.latitude_deg, 48.1173, 0.00001));
    assert(near(fix.longitude_deg, 11.5166667, 0.00001));
    assert(near(fix.altitude_m, 545.4, 0.01));
    assert(near(fix.hdop, 0.9, 0.01));
    assert(strcmp(fix.utc_time, "123519") == 0);

    const char *rmc = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
    assert(roza_gnss_parse_sentence(rmc, &fix));
    assert(fix.has_fix);
    assert(near(fix.speed_kmh, 41.4848, 0.01));
    assert(near(fix.course_deg, 84.4, 0.01));
    assert(strcmp(fix.utc_date, "230394") == 0);
    int64_t utc_epoch = 0;
    assert(roza_gnss_utc_epoch(&fix, &utc_epoch));
    assert(utc_epoch == 764426119); // 1994-03-23 12:35:19 UTC
    roza_gnss_fix_t invalid_date = fix;
    snprintf(invalid_date.utc_date, sizeof(invalid_date.utc_date), "310224");
    assert(!roza_gnss_utc_epoch(&invalid_date, &utc_epoch));

    const char *bad_checksum = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*46";
    assert(!roza_gnss_parse_sentence(bad_checksum, &fix));

    const char *unsupported = "$GPGSA,A,3,04,05,09,12,24,25,29,31,,,,1.8,1.0,1.5*33";
    assert(!roza_gnss_parse_sentence(unsupported, &fix));

    roza_gnss_fix_reset(&fix);
    const std::string invalid_lat = make_nmea_sentence(
        "GPGGA,123519,9100.000,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    assert(roza_gnss_parse_sentence(invalid_lat.c_str(), &fix));
    assert(!fix.has_fix && fix.latitude_deg == 0.0);

    roza_gnss_fix_reset(&fix);
    const std::string wrong_hemisphere = make_nmea_sentence(
        "GPGGA,123519,4807.038,E,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    assert(roza_gnss_parse_sentence(wrong_hemisphere.c_str(), &fix));
    assert(!fix.has_fix);

    roza_gnss_fix_reset(&fix);
    const std::string invalid_lon = make_nmea_sentence(
        "GPGGA,123519,4807.038,N,18100.000,E,1,08,0.9,545.4,M,46.9,M,,");
    assert(roza_gnss_parse_sentence(invalid_lon.c_str(), &fix));
    assert(!fix.has_fix && fix.longitude_deg == 0.0);

    roza_gnss_fix_reset(&fix);
    const std::string huge_coordinate = make_nmea_sentence(
        "GPGGA,123519,999999999999999999999999999.0,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    assert(roza_gnss_parse_sentence(huge_coordinate.c_str(), &fix));
    assert(!fix.has_fix);

    roza_gnss_fix_reset(&fix);
    const std::string nonfinite = make_nmea_sentence(
        "GPGGA,123519,4807.038,N,01131.000,E,1,08,NaN,INF,M,46.9,M,,");
    assert(roza_gnss_parse_sentence(nonfinite.c_str(), &fix));
    assert(fix.has_fix && fix.hdop == 0.0f && fix.altitude_m == 0.0f);

    roza_gnss_fix_reset(&fix);
    const std::string invalid_speed_course = make_nmea_sentence(
        "GPRMC,123519,A,4807.038,N,01131.000,E,NaN,999,230394,003.1,W");
    assert(roza_gnss_parse_sentence(invalid_speed_course.c_str(), &fix));
    assert(fix.has_fix && fix.speed_kmh == 0.0f && fix.course_deg == 0.0f);

    roza_gnss_fix_reset(&fix);
    assert(!fix.has_fix && fix.satellites == 0);
    puts("ROZA GNSS NMEA parser tests passed");
    return 0;
}
