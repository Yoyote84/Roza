#include "roza_launcher_hal.h"
#include "nucleo_app.h"
#include "launcher_theme.h"

#include <Arduino.h>
#include <DisplayDrivers.h>
#include <new>

namespace {
static tft_display s_display(240, 135);
static tft_sprite *s_sprite;
static bool s_target_ready;

static void display_enter()
{
    // Keep NucleoOS's normal buffered draw path. The DisplayDrivers adapter resolves
    // its target through nucleo_app_gfx(), so drawing and sprite blits land on the
    // shared app canvas; NucleoOS performs the single final panel blit.
    // This lightweight test app must not switch to direct draw or release the
    // shared 32 KiB canvas.
    s_target_ready = roza_launcher_display_target() != nullptr;
    if (!s_target_ready) {
        nucleo_app_set_hint("Cible graphique indisponible");
        return;
    }

    // Do not call tft_display::init/begin/display here. Those lifecycle hooks
    // belong to standalone Launcher; NucleoOS owns M5 initialization and frame flush.
    s_sprite = new (std::nothrow) tft_sprite(&s_display);
    if (s_sprite && s_sprite->createSprite(72, 18) == nullptr) {
        delete s_sprite;
        s_sprite = nullptr;
    }

    nucleo_app_set_hint("Q quitter  |  ESC retour");
    nucleo_app_request_draw();
}

static void display_key(int key, char ch)
{
    if (key == NK_CHAR && (ch == 'q' || ch == 'Q')) {
        nucleo_app_exit();
        return;
    }
}

static void display_draw()
{
    if (!s_target_ready) return;

    const int top = nucleo_app_content_top();
    const int height = nucleo_app_content_height();
    const int width = s_display.width();

    s_display.fillRect(0, top, width, height, BG);
    s_display.setTextSize(1);
    s_display.setTextColor(C_BLUE, BG);
    s_display.setCursor(8, top + 4);
    s_display.print("DISPLAYDRIVERS / ROZA");

    s_display.setTextColor(FG, BG);
    s_display.setCursor(8, top + 22);
    s_display.print("Cible : nucleo_app_gfx()");
    s_display.setCursor(8, top + 36);
    s_display.print("M5.begin : non appele");
    s_display.setCursor(8, top + 50);
    s_display.print("Canvas plein ecran : aucun");

    if (s_sprite) {
        s_sprite->fillScreen(C_BLUE);
        s_sprite->setTextSize(1);
        s_sprite->setTextColor(FG, C_BLUE);
        s_sprite->drawString(String("Sprite 72x18"), 2, 4);
        s_sprite->pushSprite(8, top + 68);
    } else {
        s_display.setTextColor(MUTED, BG);
        s_display.setCursor(8, top + 68);
        s_display.print("Sprite non alloue");
    }

}

static void display_exit()
{
    if (s_sprite) {
        s_sprite->deleteSprite();
        delete s_sprite;
        s_sprite = nullptr;
    }
    s_target_ready = false;
}

static const nucleo_app_def_t s_app = {
    "roza-launcher-display-backend",
    "Affichage Launcher",
    "Systeme",
    "Test du backend M5GFX partage avec NucleoOS",
    'G',
    C_BLUE,
    display_enter,
    display_key,
    nullptr,
    display_draw,
    display_exit,
    0,
    nullptr,
    nullptr
};
} // namespace

extern "C" void roza_register_launcher_display(void)
{
    nucleo_app_register(&s_app);
}
