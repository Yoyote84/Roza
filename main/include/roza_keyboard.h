#pragma once

/*
 * РОЗА — Clavier TCA8418 pour M5Stack Cardputer ADV
 * Pilote ESP-IDF natif (driver I2C) - pas de dépendance Arduino.
 * Licence : adaptation ESP-IDF, code original M5Stack/M5Cardputer MIT.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

// Configuration TCA8418 pour Cardputer ADV
#define ROZA_TCA8418_I2C_ADDR   0x34
#define ROZA_TCA8418_SDA_PIN    GPIO_NUM_8
#define ROZA_TCA8418_SCL_PIN    GPIO_NUM_9
#define ROZA_TCA8418_INT_PIN    GPIO_NUM_11

// Taille matrice TCA8418
#define ROZA_TCA8418_NROWS      7
#define ROZA_TCA8418_NCOLS      8

// Codes de touche ROZA (français, thème 1940-1950)
// Basé sur le mapping _key_value_map de M5Cardputer, adapté
#define KEY_ROZA_ESCAPE         0x01
#define KEY_ROZA_ENTER          0x1C
#define KEY_ROZA_BACKSPACE      0xE1
#define KEY_ROZA_TAB            0x0F

// Structure d'état clavier ROZA
typedef struct {
    bool key_state[4][14]; // État de chaque touche [row][col]
    uint8_t last_key_char; // Dernier caractère tapé
} roza_keyboard_state_t;

// Fonctions d'initialisation et de configuration
bool roza_keyboard_init(void);
void roza_keyboard_deinit(void);

// Fonctions de lecture d'état
roza_keyboard_state_t roza_keyboard_get_state(void);
bool roza_keyboard_key_pressed(char key_char);
bool roza_keyboard_any_key_pressed(void);

// Fonctions de traitement
void roza_keyboard_update(void);
void roza_keyboard_set_key_state(uint8_t row, uint8_t col, bool pressed);
#endif /* ROZA_KEYBOARD_H */