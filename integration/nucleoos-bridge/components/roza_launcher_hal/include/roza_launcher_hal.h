#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROZA_LAUNCHER_KEY_NONE = 0,
    ROZA_LAUNCHER_KEY_UP,
    ROZA_LAUNCHER_KEY_DOWN,
    ROZA_LAUNCHER_KEY_LEFT,
    ROZA_LAUNCHER_KEY_RIGHT,
    ROZA_LAUNCHER_KEY_ENTER,
    ROZA_LAUNCHER_KEY_ESCAPE,
    ROZA_LAUNCHER_KEY_BACKSPACE,
    ROZA_LAUNCHER_KEY_TAB,
    ROZA_LAUNCHER_KEY_CHARACTER
} roza_launcher_key_t;

#define ROZA_LAUNCHER_MOD_SHIFT 0x01
#define ROZA_LAUNCHER_MOD_CTRL  0x02
#define ROZA_LAUNCHER_MOD_ALT   0x04
#define ROZA_LAUNCHER_MOD_META  0x08
#define ROZA_LAUNCHER_MOD_FN    0x10

typedef struct {
    roza_launcher_key_t key;
    char character;
    uint8_t modifiers;
} roza_launcher_key_event_t;

bool roza_launcher_translate_key(int native_key, char native_char,
                                 uint8_t native_modifiers,
                                 roza_launcher_key_event_t *out);
bool roza_launcher_key_from_callback(int native_key, char native_char,
                                     roza_launcher_key_event_t *out);
/* Opaque pointer keeps the host-tested HAL independent of M5GFX headers. */
void *roza_launcher_display_target(void);
int roza_launcher_battery_percent(void);
int roza_launcher_battery_millivolts(void);
bool roza_launcher_storage_ready(void);
void roza_launcher_set_brightness(int percent);

#ifdef __cplusplus
}
#endif
