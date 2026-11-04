#pragma once

/*
 * ROZA — Driver LCD ST7789V2 pour ESP-IDF natif.
 * Pilote ESP-IDF utilisant driver/spi_master et GPIO.
 * Licence : adaptation ESP-IDF, code original Arduino_ST7789 MIT.
 */

#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "soc/gpio_sig_map.h"

// Configuration de l'écran ST7789V2 pour M5Stack Cardputer ADV
#define ROZA_DISPLAY_WIDTH    240
#define ROZA_DISPLAY_HEIGHT   135

// Broches M5Stack Cardputer ADV (validées documentation M5Stack)
#define ROZA_PIN_LCD_RST      GPIO_NUM_33
#define ROZA_PIN_LCD_DC       GPIO_NUM_34
#define ROZA_PIN_LCD_CS       GPIO_NUM_37
#define ROZA_PIN_LCD_SCK      GPIO_NUM_36
#define ROZA_PIN_LCD_MOSI     GPIO_NUM_35
#define ROZA_PIN_LCD_BL       GPIO_NUM_38  // Rétroéclairage

// Balises d'initialisation ST7789V2 (polynômes de configuration)
#define ROZA_TFT_IPS          true
#define ROZA_OFFSET_COL1      52
#define ROZA_OFFSET_ROW1      40
#define ROZA_OFFSET_COL2      53
#define ROZA_OFFSET_ROW2      40

// États d'affichage
typedef enum {
    ROZA_DISPLAY_STATE_ON,
    ROZA_DISPLAY_STATE_OFF,
    ROZA_DISPLAY_STATE_SLEEP
} roza_display_state_t;

// Structure de contexte écran
typedef struct {
    bool initialized;
    uint16_t backlight_brightness;
    roza_display_state_t state;
} roza_display_context_t;

/*
 * Fonctions d'initialisation et de configuration
 */
bool roza_display_init(void);
void roza_display_set_backlight(uint8_t brightness);
void roza_display_backlight_on(void);
void roza_display_backlight_off(void);
roza_display_state_t roza_display_get_state(void);

/*
 * Fonctions d'affichage de base (implémentation ESP-IDF native)
 * Ces fonctions utilisent le driver SPI master bas niveau.
 * Les séquences d'initialisation sont celles du datasheet ST7789V2.
 */
void roza_display_clear(uint16_t color);
void roza_display_draw_pixel(int16_t x, int16_t y, uint16_t color);
void roza_display_fill_screen(uint16_t color);
void roza_display_draw_horizontal_line(int16_t x, int16_t y, int16_t length, uint16_t color);
void roza_display_draw_vertical_line(int16_t x, int16_t y, int16_t length, uint16_t color);

/*
 * Fonction d'affichage de démarrage ROZA avec identité visuelle
 * Affiche l'écran de bienvenue thème années 1940-1950.
 * Utilise les registres ST7789V2 définis dans le datasheet.
 */
void roza_display_show_welcome(void);

/*
 * Nettoyage et mise à jour
 */
void roza_display_update(void);
void roza_display_deinit(void);

/*
 * Accesseurs de contexte
 */
roza_display_context_t* roza_display_get_context(void);