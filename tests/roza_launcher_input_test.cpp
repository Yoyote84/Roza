#include "roza_launcher_hal.h"
#include "nucleo_kbd.h"
#include <cassert>
#include <cstdio>

static void expect(int native_key, char native_char, unsigned char native_mods,
                   roza_launcher_key_t expected_key, char expected_char,
                   unsigned char expected_mods)
{
    roza_launcher_key_event_t event = {};
    assert(roza_launcher_translate_key(native_key, native_char, native_mods, &event));
    assert(event.key == expected_key);
    assert(event.character == expected_char);
    assert(event.modifiers == expected_mods);
}

int main()
{
    expect(NK_UP, ';', 0, ROZA_LAUNCHER_KEY_UP, ';', 0);
    expect(NK_DOWN, '.', 0, ROZA_LAUNCHER_KEY_DOWN, '.', 0);
    expect(NK_LEFT, ',', 0, ROZA_LAUNCHER_KEY_LEFT, ',', 0);
    expect(NK_RIGHT, '/', 0, ROZA_LAUNCHER_KEY_RIGHT, '/', 0);
    expect(NK_ENTER, 0, 0, ROZA_LAUNCHER_KEY_ENTER, 0, 0);
    expect(NK_BACK, 0, 0, ROZA_LAUNCHER_KEY_ESCAPE, 0, 0);
    expect(NK_DEL, '\b', 0, ROZA_LAUNCHER_KEY_BACKSPACE, '\b', 0);
    expect(NK_TAB, 0, 0, ROZA_LAUNCHER_KEY_TAB, 0, 0);
    expect(NK_CHAR, 'A', NK_MOD_SHIFT | NK_MOD_CTRL | NK_MOD_ALT | NK_MOD_GUI | NK_MOD_FN,
           ROZA_LAUNCHER_KEY_CHARACTER, 'A',
           ROZA_LAUNCHER_MOD_SHIFT | ROZA_LAUNCHER_MOD_CTRL | ROZA_LAUNCHER_MOD_ALT |
           ROZA_LAUNCHER_MOD_META | ROZA_LAUNCHER_MOD_FN);

    roza_launcher_key_event_t event = {ROZA_LAUNCHER_KEY_CHARACTER, 'x', 0xFF};
    assert(!roza_launcher_translate_key(NK_NONE, 0, 0, &event));
    assert(event.key == ROZA_LAUNCHER_KEY_NONE && event.character == 0 && event.modifiers == 0);
    assert(!roza_launcher_translate_key(999, 'x', 0, &event));
    assert(!roza_launcher_translate_key(NK_CHAR, '\n', 0, &event));
    assert(!roza_launcher_translate_key(NK_CHAR, 0, 0, &event));
    assert(!roza_launcher_translate_key(NK_UP, ';', 0, nullptr));
    std::puts("ROZA Launcher input translation: PASS");
    return 0;
}
