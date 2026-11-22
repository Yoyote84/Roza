#include "roza_gnss_checksum.h"
#include <assert.h>
#include <stdio.h>

int main()
{
    uint8_t checksum = 0;
    assert(roza_nmea_parse_checksum("47", &checksum) && checksum == 0x47);
    assert(roza_nmea_parse_checksum("aF", &checksum) && checksum == 0xAF);
    assert(roza_nmea_parse_checksum("00", &checksum) && checksum == 0x00);
    assert(roza_nmea_parse_checksum("FF", &checksum) && checksum == 0xFF);
    assert(!roza_nmea_parse_checksum("", &checksum));
    assert(!roza_nmea_parse_checksum("4", &checksum));
    assert(!roza_nmea_parse_checksum("4G", &checksum));
    assert(!roza_nmea_parse_checksum("G4", &checksum));
    assert(!roza_nmea_parse_checksum("+1", &checksum));
    assert(!roza_nmea_parse_checksum(" 1", &checksum));
    assert(!roza_nmea_parse_checksum("47x", &checksum));
    assert(!roza_nmea_parse_checksum("47*", &checksum));
    assert(!roza_nmea_parse_checksum("47\\r", &checksum));
    assert(!roza_nmea_parse_checksum("47\\n", &checksum));
    assert(!roza_nmea_parse_checksum(nullptr, &checksum));
    assert(!roza_nmea_parse_checksum("47", nullptr));
    puts("ROZA NMEA strict checksum tests passed");
    return 0;
}
