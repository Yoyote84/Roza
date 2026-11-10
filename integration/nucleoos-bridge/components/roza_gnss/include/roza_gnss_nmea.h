#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool has_fix;
    int satellites;
    float hdop;
    double latitude_deg;
    double longitude_deg;
    float altitude_m;
    float speed_kmh;
    float course_deg;
    char utc_time[16];
    char utc_date[16];
} roza_gnss_fix_t;

void roza_gnss_fix_reset(roza_gnss_fix_t *fix);
bool roza_gnss_parse_sentence(const char *sentence, roza_gnss_fix_t *fix);
bool roza_gnss_utc_epoch(const roza_gnss_fix_t *fix, int64_t *epoch_seconds);

#ifdef __cplusplus
}
#endif
