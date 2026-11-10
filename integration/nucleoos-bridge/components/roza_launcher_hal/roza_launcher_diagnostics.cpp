#include "roza_launcher_hal.h"
#include "nucleo_app.h"
#include "launcher_theme.h"
#include <M5GFX.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {
static roza_launcher_key_event_t s_last_event = {};
static unsigned long s_key_count;
static char s_key_label[32] = "Aucune";

static const char *key_name(roza_launcher_key_t key)
{
    switch (key) {
        case ROZA_LAUNCHER_KEY_UP: return "Haut";
        case ROZA_LAUNCHER_KEY_DOWN: return "Bas";
        case ROZA_LAUNCHER_KEY_LEFT: return "Gauche";
        case ROZA_LAUNCHER_KEY_RIGHT: return "Droite";
        case ROZA_LAUNCHER_KEY_ENTER: return "Entree";
        case ROZA_LAUNCHER_KEY_ESCAPE: return "Echap";
        case ROZA_LAUNCHER_KEY_BACKSPACE: return "Retour arr.";
        case ROZA_LAUNCHER_KEY_TAB: return "Tab";
        case ROZA_LAUNCHER_KEY_CHARACTER: return "Caractere";
        default: return "Aucune";
    }
}

static bool capture_event(int key, char ch)
{
    roza_launcher_key_event_t event = {};
    if (!roza_launcher_key_from_callback(key, ch, &event)) return false;
    s_last_event = event;
    ++s_key_count;
    if (event.key == ROZA_LAUNCHER_KEY_CHARACTER &&
        isprint(static_cast<unsigned char>(event.character))) {
        snprintf(s_key_label, sizeof(s_key_label), "%s : %c", key_name(event.key), event.character);
    } else {
        snprintf(s_key_label, sizeof(s_key_label), "%s", key_name(event.key));
    }
    nucleo_app_request_draw();
    return true;
}

static void diagnostic_enter(void)
{
    s_last_event = {};
    s_key_count = 0;
    snprintf(s_key_label, sizeof(s_key_label), "Aucune");
    nucleo_app_set_hint("Q quitter  |  ESC / fleche gauche tester");
    nucleo_app_set_back_handler([](int key) -> bool {
        capture_event(key, 0);
        return true;
    });
    nucleo_app_request_draw();
}

static void diagnostic_key(int key, char ch)
{
    if (key == NK_CHAR && (ch == 'q' || ch == 'Q')) {
        nucleo_app_exit();
        return;
    }
    capture_event(key, ch);
}

static void diagnostic_draw(void)
{
    LovyanGFX *display = roza_launcher_display_target();
    if (!display) return;
    const int top = nucleo_app_content_top();
    const int height = nucleo_app_content_height();
    const int width = display->width();
    const uint16_t bg = BG;
    display->setFont(&fonts::Font0);
    display->setTextSize(1);
    display->fillRect(0, top, width, height, bg);

    const int x = 7;
    int y = top + 2;
    display->setTextColor(C_BLUE, bg);
    display->setCursor(x, y);
    display->print("DIAGNOSTIC ROZA");
    y += 15;

    display->setTextColor(FG, bg);
    display->setCursor(x, y);
    display->print("Clavier : ");
    display->print(s_key_label);
    y += 14;

    char modifiers[40] = "";
    const char *separator = "";
    if (s_last_event.modifiers & ROZA_LAUNCHER_MOD_SHIFT) { strcat(modifiers, "Shift"); separator = " "; }
    if (s_last_event.modifiers & ROZA_LAUNCHER_MOD_CTRL) { strcat(modifiers, separator); strcat(modifiers, "Ctrl"); separator = " "; }
    if (s_last_event.modifiers & ROZA_LAUNCHER_MOD_ALT) { strcat(modifiers, separator); strcat(modifiers, "Alt"); separator = " "; }
    if (s_last_event.modifiers & ROZA_LAUNCHER_MOD_META) { strcat(modifiers, separator); strcat(modifiers, "Meta"); separator = " "; }
    if (s_last_event.modifiers & ROZA_LAUNCHER_MOD_FN) { strcat(modifiers, separator); strcat(modifiers, "Fn"); }
    if (!modifiers[0]) snprintf(modifiers, sizeof(modifiers), "Aucun");
    display->setCursor(x, y);
    display->print("Modif. : ");
    display->print(modifiers);
    y += 14;

    const int battery = roza_launcher_battery_percent();
    const int millivolts = roza_launcher_battery_millivolts();
    char battery_text[40];
    if (battery >= 0 && millivolts > 0)
        snprintf(battery_text, sizeof(battery_text), "%d%%  %d mV", battery, millivolts);
    else if (battery >= 0)
        snprintf(battery_text, sizeof(battery_text), "%d%%", battery);
    else
        snprintf(battery_text, sizeof(battery_text), "Indisponible");
    display->setCursor(x, y);
    display->print("Batterie : ");
    display->print(battery_text);
    y += 14;

    display->setCursor(x, y);
    display->print("MicroSD : ");
    display->print(roza_launcher_storage_ready() ? "Montee" : "Absente");
    y += 14;

    char count_text[32];
    snprintf(count_text, sizeof(count_text), "Appuis recus : %lu", s_key_count);
    display->setTextColor(MUTED, bg);
    display->setCursor(x, y);
    display->print(count_text);
}

static const nucleo_app_def_t s_app = {
    "roza-launcher-diagnostics", "Diagnostic ROZA", "Systeme",
    "Controle clavier, ecran, batterie et SD", 'D', C_BLUE,
    diagnostic_enter, diagnostic_key, nullptr, diagnostic_draw, nullptr,
    0, nullptr, nullptr
};
} // namespace

extern "C" void roza_register_launcher_diagnostics(void)
{
    nucleo_app_register(&s_app);
}
