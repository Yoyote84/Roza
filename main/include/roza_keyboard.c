#include "roza_keyboard.h"

#include "driver/i2c.h"
#include "esp_log.h"

// Adresse I2C TCA8418 sur Cardputer ADV
#define ROZA_TCA8418_I2C_ADDR  0x34

// Registres TCA8418
#define TCA8418_REG_INPUT0     0x00
#define TCA8418_REG_INPUT1     0x01
#define TCA8418_REG_OUTPUT     0x02
#define TCA8418_REG_POLINV     0x03
#define TCA8418_REG_POLINV2    0x04
#define TCA8418_REG_CONFIG0    0x05
#define TCA8418_REG_CONFIG1    0x06
#define TCA8418_REG_DEBOUNCE   0x07
#define TCA8418_REG_INT_STAT   0x0F

// État global
static i2c_port_t g_i2c_port = I2C_NUM_0;
static uint8_t g_key_state[4][14] = {{false}};
static uint8_t g_last_key_char = 0;

/*
 * @brief Initialisation du clavier TCA8418 sur Cardputer ADV
 * Utilise le driver I2C natif ESP-IDF.
 * @return true si initialization réussie
 */
bool roza_keyboard_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = ROZA_TCA8418_SDA_PIN,
        .scl_io_num = ROZA_TCA8418_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100 kHz I2C standard
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    // Configuration TCA8418 : matrice 7x8, interruptions désactivées pour polling
    uint8_t config0 = 0x00; // Toutes les broches en entrée par défaut
    uint8_t config1 = 0x00;
    uint8_t debounce = 0x00; // Pas de debounce logiciel

    // Écriture des registres de configuration
    i2c_master_write_to_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &config0, 1, pdMS_TO_TICKS(1000));
    i2c_master_write_to_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &config1, 1, pdMS_TO_TICKS(1000));
    i2c_master_write_to_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &debounce, 1, pdMS_TO_TICKS(1000));

    ESP_LOGI("ROZA_KEYBOARD", "TCA8418 initialized via I2C native ESP-IDF");
    return true;
}

/*
 * @brief Désinitialisation du clavier
 */
void roza_keyboard_deinit(void) {
    i2c_driver_delete(I2C_NUM_0);
}

/*
 * @brief Lecture de l'état actuel du clavier
 * @return Structure d'état avec touches appuyées
 */
roza_keyboard_state_t roza_keyboard_get_state(void) {
    roza_keyboard_state_t state;
    state.last_key_char = g_last_key_char;

    // Copier l'état des touches
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 14; c++) {
            state.key_state[r][c] = g_key_state[r][c];
        }
    }
    return state;
}

/*
 * @brief Vérifie si une touche spécifique est appuyée
 */
bool roza_keyboard_key_pressed(char key_char) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 14; c++) {
            if (g_key_state[r][c]) {
                // Mapping simplifié : chaque position a un caractère associé
                // Dans une implémentation complète, on comparerait key_char
                // avec la valeur attendue pour position (r,c)
                return true; // Placeholder - mapping complet requis
            }
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
            if (g_key_state[r][c]) {
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
    // Lire les registres d'entrée TCA8418
    uint8_t input0 = 0, input1 = 0;
    esp_err_t ret = i2c_master_read_from_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &input0, 1, pdMS_TO_TICKS(1000));
    if (ret == ESP_OK) {
        // input0 contient les états des touches
        // Décodage du TCA8418 : chaque bit correspond à une touche
        // Mapping simplifié : on lit les bits et on met à jour g_key_state
        for (int i = 0; i < 8; i++) {
            bool pressed = (input0 >> i) & 1;
            // Mapping vers notre grille 4x14 - à affiner selon la carte réelle
            if (i < 4) {
                g_key_state[0][i] = pressed; // Ligne 0
            } else if (i < 8) {
                g_key_state[1][i-4] = pressed; // Ligne 1
            }
        }
    }

    // Lire le registre d'état d'interruption
    uint8_t int_stat = 0;
    i2c_master_write_to_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &TCA8418_REG_INT_STAT, 1, pdMS_TO_TICKS(1000));
    i2c_master_read_from_device(I2C_NUM_0, ROZA_TCA8418_I2C_ADDR, &int_stat, 1, pdMS_TO_TICKS(1000));
    // En mode polling, pas de nettoyage d'interruption nécessaire obligatoirement

    ESP_LOGD("ROZA_KEYBOARD", "Keyboard update completed");
}

/*
 * @defini l'état d'une touche spécifique
 * @param row Ligne (0-3)
 * @param col Colonne (0-13)
 * @param pressed true si appuyée, false si relâchée
 */
void roza_keyboard_set_key_state(uint8_t row, uint8_t col, bool pressed) {
    if (row < 4 && col < 14) {
        g_key_state[row][col] = pressed;
    }
}

/*
 * @brief Obtenir le caractère du dernier événement
 * @return Caractère lu, ou 0 s'il n'y en a pas
 */
uint8_t roza_keyboard_get_last_char(void) {
    return g_last_key_char;
}