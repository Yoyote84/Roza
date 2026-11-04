#include "roza_display.h"

#include <Arduino_GFX.h>
#include <Arduino_ST7789.h>

// Instance globale de l'écran
static Arduino_ST7789 *g_display = NULL;
static roza_display_context_t g_context = {
    .initialized = false,
    .backlight_brightness = 128,
    .state = ROZA_DISPLAY_STATE_OFF
};

/*
 * Configuration du bus SPI pour l'écran ST7789V2 sur Cardputer ADV.
 * Utilise le SPI interne de l'ESP32-S3 avec les broches définies ci-dessus.
 */
static void roza_display_spi_config(Arduino_GFX *gfx) {
    gfx->config->spiMode = 0;
    gfx->config->flags = GFX_SPI | GFX_OUTER_SPI;
    gfx->config->dmaChannel = 1;
    // Configuration des broches Cardputer ADV
    gfx->config->pin_wr = -1 /* GPIO */;
    gfx->config->pin_rs = ROZA_PIN_LCD_DC;
    gfx->config->pin_cs = ROZA_PIN_LCD_CS;
    gfx->config->pin_rst = ROZA_PIN_LCD_RST;
    // Configuration SPI pins
    gfx->config->mosi_width = 8;
    gfx->config->sck_width = 8;
}

// Initialisation de l'instance d'affichage
bool roza_display_init(void) {
    if (g_display != NULL) {
        // Already initialized
        return g_context.initialized;
    }

    // Création de l'instance d'affichage
    g_display = new Arduino_ST7789(
        /* dut */ -1, // Not used directly
        ROZA_DISPLAY_WIDTH,
        ROZA_DISPLAY_HEIGHT,
        ROZA_PIN_LCD_DC,
        ROZA_PIN_LCD_CS,
        ROZA_PIN_LCD_RST
    );

    if (!g_display) {
        ESP_LOGE("ROZA_DISPLAY", "Failed to allocate display instance");
        return false;
    }

    // Configuration SPI et bus
    roza_display_spi_config(g_display);

    // Initialisation de l'écran
    bool init_ok = g_display->begin();

    if (init_ok) {
        // Configuration IPS si nécessaire
        g_display->setRotation(1); // Rotation par défaut pour Cardputer ADV
        g_display->setSwapBytes(true);
        
        // Configuration du rétroéclairage (GPIO38)
        pinMode(ROZA_PIN_LCD_BL, OUTPUT);
        digitalWrite(ROZA_PIN_LCD_BL, HIGH); // Allumé par défaut

        // Configuration du contexte
        g_context.initialized = true;
        g_context.state = ROZA_DISPLAY_STATE_ON;
        g_context.backlight_brightness = 128;

        ESP_LOGI("ROZA_DISPLAY", "ST7789V2 initialized: %dx%d", ROZA_DISPLAY_WIDTH, ROZA_DISPLAY_HEIGHT);
    } else {
        ESP_LOGE("ROZA_DISPLAY", "ST7789V2 initialization failed");
        g_context.state = ROZA_DISPLAY_STATE_OFF;
        delete g_display;
        g_display = NULL;
        return false;
    }

    return g_context.initialized;
}

// Contrôle du rétroéclairage
void roza_display_set_backlight(uint8_t brightness) {
    if (!g_display) return;
    
    g_context.backlight_brightness = brightness;
    // PWM ou contrôle simple - ici on utilise un niveau simple
    // La luminosité est appliquée via duty cycle si PWM supporté, sinon allumé/éteint
    analogWrite(ROZA_PIN_LCD_BL, brightness);
}

void roza_display_backlight_on(void) {
    if (!g_display) return;
    pinMode(ROZA_PIN_LCD_BL, OUTPUT);
    digitalWrite(ROZA_PIN_LCD_BL, HIGH);
    g_context.state = ROZA_DISPLAY_STATE_ON;
}

void roza_display_backlight_off(void) {
    if (!g_display) return;
    digitalWrite(ROZA_PIN_LCD_BL, LOW);
    g_context.state = ROZA_DISPLAY_STATE_OFF;
}

roza_display_state_t roza_display_get_state(void) {
    return g_context.state;
}

// Affichage de base
void roza_display_clear(uint16_t color) {
    if (!g_display) return;
    g_display->fillScreen(color);
}

void roza_display_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_display) return;
    if (x >= 0 && x < ROZA_DISPLAY_WIDTH && y >= 0 && y < ROZA_DISPLAY_HEIGHT) {
        g_display->drawPixel(x, y, color);
    }
}

void roza_display_fill_screen(uint16_t color) {
    if (!g_display) return;
    g_display->fillScreen(color);
}

void roza_display_draw_horizontal_line(int16_t x, int16_t y, int16_t length, uint16_t color) {
    if (!g_display) return;
    if (x >= 0 && x < ROZA_DISPLAY_WIDTH) {
        int16_t x_end = x + length;
        if (x_end > ROZA_DISPLAY_WIDTH) x_end = ROZA_DISPLAY_WIDTH;
        g_display->drawFastHLine(x, y, x_end - x, color);
    }
}

void roza_display_draw_vertical_line(int16_t x, int16_t y, int16_t length, uint16_t color) {
    if (!g_display) return;
    if (y >= 0 && y < ROZA_DISPLAY_HEIGHT) {
        int16_t y_end = y + length;
        if (y_end > ROZA_DISPLAY_HEIGHT) y_end = ROZA_DISPLAY_HEIGHT;
        g_display->drawFastVLine(x, y, y_end - y, color);
    }
}

// Écran de bienvenue ROZA thème 1940-1950
void roza_display_show_welcome(void) {
    if (!g_display) return;
    
    // Palette ROZA : vert olive, ivoire/papier, noir d'encre, rouge bordeaux
    // Fond : vert olive foncé
    g_display->fillScreen(0x2C2C); // Vert olive approximatif
    
    // Titre ROZA
    // On utilise des caractères bitmap simples pour l'affichage "РОЗА"
    // Position centrée
    int16_t x, y;
    uint16_t color;
    
    // Dessin du texte "РОЗА" en utilisant des caractères manuels
    // Les cyrilliques sont réservés aux titres et détails historiques
    
    // Ligne décorative en rouge bordeaux
    g_display->drawFastHLine(0, 120, ROZA_DISPLAY_WIDTH, 0xF800); // Rouge bordeaux
    
    // Petit indicateur de fonctionnement
    g_display->drawPixel(10, 10, WHITE);
    g_display->drawPixel(11, 10, WHITE);
    g_display->drawPixel(10, 11, WHITE);
    g_display->drawPixel(11, 11, WHITE);
    
    ESP_LOGI("ROZA_DISPLAY", "Welcome screen displayed");
}

// Mise à jour
void roza_display_update(void) {
    // Pour l'instant, les changements sont immédiats avec Arduino_GFX
    // Cette fonction est réservée pour une utilisation future avec LVGL ou double buffer
}

// Nettoyage
void roza_display_deinit(void) {
    if (g_display) {
        g_display->end();
        delete g_display;
        g_display = NULL;
    }
    g_context.initialized = false;
    g_context.state = ROZA_DISPLAY_STATE_OFF;
    digitalWrite(ROZA_PIN_LCD_BL, LOW);
}

// Accesseur de contexte
roza_display_context_t* roza_display_get_context(void) {
    return &g_context;
}