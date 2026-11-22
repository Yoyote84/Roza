#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline int roza_nmea_hex_nibble(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
}

static inline bool roza_nmea_parse_checksum(const char *suffix, uint8_t *checksum)
{
    if (!suffix || !checksum || suffix[0] == '\0' || suffix[1] == '\0' || suffix[2] != '\0')
        return false;
    const int high = roza_nmea_hex_nibble(suffix[0]);
    const int low = roza_nmea_hex_nibble(suffix[1]);
    if (high < 0 || low < 0) return false;
    *checksum = static_cast<uint8_t>((high << 4) | low);
    return true;
}
