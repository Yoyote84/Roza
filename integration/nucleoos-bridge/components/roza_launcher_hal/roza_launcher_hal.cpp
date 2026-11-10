#include "roza_launcher_hal.h"
#include "app_gfx.h"
#include "nucleo_app.h"
#include "nucleo_kbd.h"
#include "nucleo_power.h"
#include "nucleo_storage.h"

extern "C" void *roza_launcher_display_target(void)
{
    return static_cast<void *>(nucleo_app_gfx());
}

extern "C" bool roza_launcher_key_from_callback(
    int native_key, char native_char, roza_launcher_key_event_t *out)
{
    return roza_launcher_translate_key(native_key, native_char, nucleo_kbd_mods(), out);
}

extern "C" int roza_launcher_battery_percent(void)
{
    return nucleo_power_battery_pct();
}

extern "C" int roza_launcher_battery_millivolts(void)
{
    return nucleo_power_battery_mv();
}

extern "C" bool roza_launcher_storage_ready(void)
{
    const nucleo_storage_info_t *info = nucleo_storage_info();
    return info != nullptr && info->mounted;
}

extern "C" void roza_launcher_set_brightness(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    nucleo_app_set_brightness(percent);
}
