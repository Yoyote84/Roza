#include "roza_keyboard.h"

#include <Wire.h>
#include <Adafruit_TCA8418.h>

// Instance globale du clavier TCA8418
static Adafruit_TCA8418 tca;
static roza_keyboard_state_t g_keyboard_state = {0};
static bool g_keyboard_initialized = false;

// Mapping inverse : position logique -> registre TCA8418
// Le code Launcher utilise : u = keyvalue % 10, t = keyvalue / 10
// Puis : row = u0 & 0x03, col = (t << 1) | (u0 >> 2), u0 = u - 1

/*
 * @brief Initialisation du clavier TCA8418 sur Cardputer ADV
 * @return true si initialization réussie
 */
bool roza_keyboard_init(void) {
    // Initialisation I2C avec broches Cardputer ADV
    Wire.begin(ROZA_TCA8418_SDA_PIN, ROZA_TCA8418_SCL_PIN);

    // Balayage I2C optionnel - détecter les appareils
    // (comme dans Launcher interface.cpp)

    // Initialisation du TCA8418
    if (!tca.begin(ROZA_TCA8418_I2C_ADDR, &Wire)) {
        ESP_LOGE("ROZA_KEYBOARD", "Failed to initialize TCA8418 at address 0x%02X", ROZA_TCA8418_I2C_ADDR);
        return false;
    }

    // Configuration matrice 7×8
    tca.matrix(ROZA_TCA8418_NROWS, ROZA_TCA8418_NCOLS);
    tca.flush();

    // Configurer les broches
    pinMode(ROZA_TCA8418_INT_PIN, INPUT);

    // État initial : toutes touches relâchées
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 14; c++) {
            g_keyboard_state.key_state[r][c] = false;
        }
    }
    g_keyboard_state.fn_active = false;
    g_keyboard_state.shift_active = false;
    g_keyboard_state.caps_active = false;
    g_keyboard_state.last_key_char = 0;

    g_keyboard_initialized = true;
    ESP_LOGI("ROZA_KEYBOARD", "TCA8418 initialized successfully at address 0x%02X", ROZA_TCA8418_I2C_ADDR);
    return true;
}

/*
 * @brief Désinitialisation du clavier
 */
void roza_keyboard_deinit(void) {
    if (g_keyboard_initialized) {
        g_keyboard_initialized = false;
    }
    // Nothing else to deinit for TCA8418
}

/*
 * @brief Lecture de l'état actuel du clavier
 * @return Structure d'état avec touches appuyées
 */
roza_keyboard_state_t roza_keyboard_get_state(void) {
    return g_keyboard_state;
}

/*
 * @brief Vérifie si une touche spécifique est appuyée
 */
bool roza_keyboard_key_pressed(char key_char) {
    // Parcourir la map de touches pour trouver le caractère
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 14; c++) {
            // La map contient les valeurs first et second (avec shift)
            // On vérifie d'abord sans shift
            if (roza_key_map[r][c] == key_char) {
                return g_keyboard_state.key_state[r][c];
            }
            // Ensuite avec shift (value_second)
            // Dans notre map simplifié, on considère que la première valeur
            // est la valeur de base
        }
    }
    return false;
}

/*
 * @brief Vérifie si une touche est pressée (n'importe quelle)
 */
bool roza_keyboard_any_key_pressed(void) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 14; c++) {
            if (g_keyboard_state.key_state[r][c]) {
                return true;
            }
        }
    }
    return false;
}

/*
 * @brief Met à jour l'état du clavier en lisant le TCA8418
 * Cette fonction doit être appelée périodiquement (tous les ~100ms)
 * comme dans le projet Launcher.
 */
void roza_keyboard_update(void) {
    if (!g_keyboard_initialized) return;

    // Lire l'événement du TCA8418
    // Le code Launcher utilise tca.available() et tca.getEvent()
    // Ici, on utilise la méthode polling avec délai

    // Lire les événements disponibles
    if (tca.available()) {
        int keyEvent = tca.getEvent();
        bool pressed = (keyEvent & 0x80); // Bit 7: 1 = Pressed, 0 = Released
        uint8_t value = keyEvent & 0x7F;  // Bits 0-6: value 1-127

        // Mapper la valeur brute vers position physique
        // Code Launcher : u = value % 10 (1..8), t = value / 10 (0..6)
        //                 row = (u - 1) & 0x03, col = (t << 1) | ((u - 1) >> 2)
        uint8_t u = value % 10;      // 1..8
        uint8_t t = value / 10;      // 0..6

        if (u >= 1 && u <= 8 && t <= 6) {
            uint8_t u0 = u - 1;      // 0..7
            uint8_t row = u0 & 0x03; // bits [1:0] => 0..3
            uint8_t col = (t << 1) | (u0 >> 2); // t*2 + bit2(u0) => 0..13

            // S'assurer que row/col sont dans les bornes
            if (row < 4 && col < 14) {
                // Mettre à jour l'état de la touche
                bool new_state = pressed;
                if (g_keyboard_state.key_state[row][col] != new_state) {
                    g_keyboard_state.key_state[row][col] = new_state;

                    // Traiter les touches spéciales
                    char keyChar = roza_key_map[row][col];

                    if (pressed) {
                        // Mise à jour du caractère dernier pressé
                        g_keyboard_state.last_key_char = keyChar;
                    }
                }
            }
        }
    }

    // Lire et effacer le drapeau d'interruption
    // Le code Launcher utilise : tca.writeRegister(TCA8418_REG_INT_STAT, 1)
    // Mais en mode polling, on lit juste l'état
    int intstat = tca.readRegister(0x03); // REG_INT_STAT
    // Aucun action nécessaire en mode polling pur

    // Détecter la touche Fn pour basculer les couches
    // Vérifier si Fn est appuyé (position dépend de la mapping)
    // Dans notre map, Fn est à [3][0] et [0][0] selon les passes
    // On vérifie simplement si le caractère '`' ou la valeur spécifique est pressé

    // Mise à jour de l'état Fn, Shift, Caps via gestion dans roza_main.c
    // Pour l'instant, on lit simplement les états boutons
}

/*
 * @basculer la couche Fn (couche supérieure des touches)
 */
void roza_keyboard_set_fn_layer(bool active) {
    g_keyboard_state.fn_active = active;
    ESP_LOGD("ROZA_KEYBOARD", "FN layer %s", active ? "activated" : "deactivated");
}

/*
 * @basculer l'état Shift
 */
void roza_keyboard_set_shift(bool active) {
    g_keyboard_state.shift_active = active;
}

/*
 * @basculer l'état Caps Lock
 */
void roza_keyboard_set_caps_lock(bool toggle) {
    g_keyboard_state.caps_active = toggle;
    ESP_LOGI("ROZA_KEYBOARD", "Caps Lock %s", toggle ? "ON" : "OFF");
}

/*
 * @brief Lecture du caractère du dernier événement
 * @return Caractère lu, ou 0 s'il n'y en a pas
 */
char roza_keyboard_get_last_char(void) {
    return g_keyboard_state.last_key_char;
}