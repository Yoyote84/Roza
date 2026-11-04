#pragma once

/*
 * РОЗА — Clavier TCA8418 pour M5Stack Cardputer ADV
 * Licence : MIT (code original de M5Stack/M5Cardputer, réutilisé avec attribution)
 * 
 * Driver pour le contrôleur clavier I2C TCA8418 tel qu'utilisé sur Cardputer ADV.
 * Adapté de la bibliothèque M5Cardputer (https://github.com/m5stack/M5Cardputer)
 * Voir aussi : Launcher project boards/m5stack-cardputer/ interface.cpp
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Configuration TCA8418 pour Cardputer ADV (depuis Launcher & M5Cardputer)
#define ROZA_TCA8418_I2C_ADDR   0x34
#define ROZA_TCA8418_SDA_PIN    8
#define ROZA_TCA8418_SCL_PIN    9
#define ROZA_TCA8418_INT_PIN    11

// Taille matrice TCA8418
#define ROZA_TCA8418_NROWS      7
#define ROZA_TCA8418_NCOLS      8

// Codes de touche ROZA (français, thème 1940-1950)
// Basé sur le map _key_value_map de M5Cardputer, adapté pour cyrillique minimal
#define KEY_ROZA_ESCAPE         0x01
#define KEY_ROZA_ENTER          0x1C
#define KEY_ROZA_BACKSPACE      0xE1
#define KEY_ROZA_TAB            0x0F
#define KEY_ROZA_SHIFT          0xE1 // Géré via état
#define KEY_ROZA_CAPS_LOCK      0xE3 // Géré via état
#define KEY_ROZA_FN             0xFF // Fn layer

// Couleurs pour clavier (palette 1940-1950)
#define KEY_COLOR_NORMAL        0xC618  // Brun olive (touch normal)
#define KEY_COLOR_ACTIVE        0xFD20  // Rouge bord (touch actif)
#define KEY_COLOR_FN            0xFE9A  // Orange (couche Fn)
#define KEY_COLOR_DISABLED      0x51A1  // Gris doux (inactif)

// Structure d'état clavier ROZA
typedef struct {
    bool fn_active;
    bool shift_active;
    bool caps_active;
    bool key_state[4][14]; // État de chaque touche [row][col]
    char last_key_char; // Dernier caractère tapé
} roza_keyboard_state_t;

// Fonctions d'initialisation
bool roza_keyboard_init(void);
void roza_keyboard_deinit(void);

// Fonctions de lecture d'état
roza_keyboard_state_t roza_keyboard_get_state(void);
bool roza_keyboard_key_pressed(char key_char);
bool roza_keyboard_any_key_pressed(void);

// Fonctions de traitement
void roza_keyboard_update(void);
void roza_keyboard_set_fn_layer(bool active);
void roza_keyboard_set_shift(bool active);
void roza_keyboard_set_caps_lock(bool toggle);

// Mapping de touches ROZA (layout 7×8 pour TCA8418)
// Valeurs: row 0-6, col 0-7
// Basé sur le code M5Cardputer TCA8418KeyboardReader avec remappage
static const uint8_t roza_key_map[4][14] = {
    // Row 0: Échap, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, -, =, Backspace
    {KEY_ROZA_ESCAPE, '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', KEY_ROZA_BACKSPACE},
    // Row 1: Tab, q, w, e, r, t, y, u, i, o, p, [, ], Backslash
    {KEY_ROZA_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\'},
    // Row 2: Fn, Shift, a, s, d, f, g, h, j, k, l, ;, "', Enter
    {KEY_ROZA_FN, KEY_ROZA_SHIFT, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', KEY_ROZA_ENTER},
    // Row 3: Ctrl, Opt, Alt, z, x, c, v, b, n, m, ,, ., /, (vide)
    {KEY_ROZA_FN, KEY_ROZA_SHIFT, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_ROZA_FN}
};

// Mapping de correspondance physique vers position logique
// TCA8418 matrix is 7 rows × 8 cols, but ROZA uses simplified 4×14 layout
// The Launcher code maps raw values: u = keyvalue % 10 (1..8), t = keyvalue / 10 (0..6)
// Then: row = u0 & 0x03, col = (t << 1) | (u0 >> 2)
// where u0 = u - 1

// Codes spéciaux ROZA
#define ROZA_KEY_NAV_LEFT     ','   // Précédent
#define ROZA_KEY_NAV_RIGHT    '.'   // Suivant
#define ROZA_KEY_NAV_UP       ';'   // Previous (up arrow equivalent)
#define ROZA_KEY_NAV_DOWN     '/'   // Next (down arrow equivalent)
#define ROZA_KEY_NAV_ENTER    KEY_ROZA_ENTER // Select/Enter
#define ROZA_KEY_NAV_ESCAPE  '`'    // Back/Escape

#endif /* ROZA_KEYBOARD_H */