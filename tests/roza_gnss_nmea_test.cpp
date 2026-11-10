#include "roza_gnss_nmea.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(double actual, double expected, double tolerance)
{
    return fabs(actual - expected) <= tolerance;
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

    const char *bad_checksum = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*46";
    assert(!roza_gnss_parse_sentence(bad_checksum, &fix));

    const char *unsupported = "$GPGSA,A,3,04,05,09,12,24,25,29,31,,,,1.8,1.0,1.5*33";
    assert(!roza_gnss_parse_sentence(unsupported, &fix));

    roza_gnss_fix_reset(&fix);
    assert(!fix.has_fix && fix.satellites == 0);
    puts("ROZA GNSS NMEA parser tests passed");
    return 0;
}
