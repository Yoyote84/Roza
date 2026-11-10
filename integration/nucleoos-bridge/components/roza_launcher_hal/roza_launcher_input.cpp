#include "roza_launcher_hal.h"
#include "nucleo_kbd.h"

bool roza_launcher_translate_key(int native_key, char native_char,
                                 uint8_t native_modifiers,
                                 roza_launcher_key_event_t *out)
{
    if (!out) return false;
    out->key = ROZA_LAUNCHER_KEY_NONE;
    out->character = 0;
    out->modifiers = 0;

    if (native_modifiers & NK_MOD_SHIFT) out->modifiers |= ROZA_LAUNCHER_MOD_SHIFT;
    if (native_modifiers & NK_MOD_CTRL)  out->modifiers |= ROZA_LAUNCHER_MOD_CTRL;
    if (native_modifiers & NK_MOD_ALT)   out->modifiers |= ROZA_LAUNCHER_MOD_ALT;
    if (native_modifiers & NK_MOD_GUI)   out->modifiers |= ROZA_LAUNCHER_MOD_META;
    if (native_modifiers & NK_MOD_FN)    out->modifiers |= ROZA_LAUNCHER_MOD_FN;

    switch (native_key) {
        case NK_UP:    out->key = ROZA_LAUNCHER_KEY_UP; break;
        case NK_DOWN:  out->key = ROZA_LAUNCHER_KEY_DOWN; break;
        case NK_LEFT:  out->key = ROZA_LAUNCHER_KEY_LEFT; break;
        case NK_RIGHT: out->key = ROZA_LAUNCHER_KEY_RIGHT; break;
        case NK_ENTER: out->key = ROZA_LAUNCHER_KEY_ENTER; break;
        case NK_BACK:  out->key = ROZA_LAUNCHER_KEY_ESCAPE; break;
        case NK_DEL:   out->key = ROZA_LAUNCHER_KEY_BACKSPACE; break;
        case NK_TAB:   out->key = ROZA_LAUNCHER_KEY_TAB; break;
        case NK_CHAR:
            if (static_cast<unsigned char>(native_char) < 32 || native_char == 127)
                return false;
            out->key = ROZA_LAUNCHER_KEY_CHARACTER;
            break;
        default:
            return false;
    }
    out->character = native_char;
    return true;
}
