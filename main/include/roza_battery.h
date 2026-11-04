#pragma once

/*
 * РОЗА — Mesure de batterie ADC pour M5Stack Cardputer ADV
 * Licence : Domaine public / adapté de la documentation ESP-IDF
 * 
 * Driver pour la lecture de la tension batterie via entrée ADC1 du ESP32-S3.
 * La broche G10 est configurée comme entrée ADC1_0 sur ESP32-S3.
 * Voir : roza_board_pins.h définit ROZA_PIN_BATTERY_ADC 10
 */

#pragma once

#include <stdint.h>

// Configuration ADC batterie ROZA
#define ROZA_BATTERY_ADC_CHANNEL  ADC1_CHANNEL_0  // G10 sur ESP32-S3
#define ROZA_BATTERY_ADC_WIDTH     ADC_WIDTH_BIT_12  // 12-bit resolution
#define ROZA_BATTERY_ATTEN         ADC_ATTEN_DB_11   // 11dB attenuation (0-150V approx)
#define ROZA_BATTERY_REF_VOLT      3.3              // Vréf ESP32-S3 interne
#define ROZA_BATTERY_DIVIDER_R1    100.0            // Résistance diviseur externe R1 (kΩ)
#define ROZA_BATTERY_DIVIDER_R2    15.0             // Résistance diviseur externe R2 (kΩ)
#define ROZA_BATTERY_MAX_V         18.0             // Tension maximale attendue (V)
#define ROZA_BATTERY_MIN_V         3.0              // Tension minimale (vide)

// État de la batterie ROZA
typedef struct {
    float voltage;        // Tension lue (V)
    float percentage;     // Niveau en % (0-100)
    bool charging;        // En charge (détecté par seuil)
    uint32_t last_read_ms; // Timestamp dernier read
} roza_battery_state_t;

/*
 * @brief Initialisation de l'ADC batterie
 * @return true si initialization réussie
 */
bool roza_battery_init(void);

/*
 * @brief Lecture de l'état batterie actuel
 * @return Structure avec tension, pourcentage, statut charge
 */
roza_battery_state_t roza_battery_get_state(void);

/*
 * @brief Calibration du rapport de diviseur
 * @param r1 Résistance supérieure (kΩ)
 * @param r2 Résistance inférieure (kΩ)
 */
void roza_battery_calibrate_divider(float r1, float r2);

/*
 * @brief Conversion valeur ADC en tension réelle
 * @param adc_val Valeur brute ADC (0-4095 pour 12-bit)
 * @return Tension en volts
 */
float roza_battery_adc_to_voltage(uint32_t adc_val);

/*
 * @brief Mise à jour de l'état batterie (à appeler périodiquement)
 */
void roza_battery_update(void);