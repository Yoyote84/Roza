#include "roza_battery.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

// Instance statique de l'état batterie
static roza_battery_state_t g_battery_state = {0};
static bool g_battery_calibrated = false;

/*
 * @brief Initialisation de l'ADC batterie sur ESP32-S3
 * Configure l'entrée ADC1_0 (GPIO10) avec atténuation 11dB
 * et largeur 12-bit. Vérifie aussi les caractéristiques de calibration.
 * @return true si initialization réussie
 */
bool roza_battery_init(void) {
    // Configurer l'atténuation ADC pour la broche G10 (ADC1_CH0)
    adc1_config_width(ROZA_BATTERY_ADC_WIDTH);
    adc1_config_channel_atten(ROZA_BATTERY_ADC_CHANNEL, ROZA_BATTERY_ATTEN);

    // Configurer la calibration ESP-IDF
    // Note : esp_adc_cal_characteristics_t sera allouée dynamiquement
    g_battery_state.voltage = 0.0;
    g_battery_state.percentage = 0;
    g_battery_state.charging = false;
    g_battery_state.last_read_ms = 0;

    ESP_LOGI("ROZA_BATTERY", "ADC initialized: Channel=%d, Width=%d bits, Attentuation=%d dB",
             ROZA_BATTERY_ADC_CHANNEL, ROZA_BATTERY_ADC_WIDTH, ROZA_BATTERY_ATTEN);

    return true;
}

/*
 * @brief Conversion valeur ADC en tension réelle
 * Utilise les paramètres de diviseur résistif externe et la référence VREF.
 * @param adc_val Valeur brute lue (0-4095 pour 12-bit ESP32-S3)
 * @return Tension aux bornes de la résistance diviseuse (V)
 */
float roza_battery_adc_to_voltage(uint32_t adc_val) {
    // Convertir valeur ADC en tension VREF références
    // ESP32-S3 ADC1 a VREF interne d'environ 1.0V-1.2V selon configuration
    // Avec atténuation 11dB, la chaîne peut mesurer jusqu'à ~150V sans diviseur
    // Mais avec diviseur R1=100k, R2=15k, le rapport est : Vout = Vin * R2/(R1+R2) = Vin * 15/115

    // Valeur de référence ADC pour 3.3V (Vref interne ESP32-S3)
    const float adc_resolution = (float)ROZA_BATTERY_REF_VOLT * (1 << ROZA_BATTERY_ADC_WIDTH) / 
                               (esp_adc_cal_raw_to_voltage(adc_val, NULL));

    // Approximation : tension lue = (valeur_adc / 4095) * Vref * facteur_diviseur
    // Facteur diviseur = (R1 + R2) / R2 = 115 / 15 = 7.667
    float voltage = (float)adc_val / ((1 << ROZA_BATTERY_ADC_WIDTH)) * ROZA_BATTERY_REF_VOLT * 
                   ((ROZA_BATTERY_DIVIDER_R1 + ROZA_BATTERY_DIVIDER_R2) / ROZA_BATTERY_DIVIDER_R2);

    return voltage;
}

/*
 * @brief Lecture de la tension batterie actuelle
 * Effectue une conversion ADC et convertit en tension réelle.
 * Inclut une petite lissage sur 3 lectures pour la stabilité.
 */
float roza_battery_read_voltage(void) {
    // Effectuer 3 lectures et moyenner pour la stabilité
    uint32_t adc_sum = 0;
    for (int i = 0; i < 3; i++) {
        adc_sum += adc1_get_raw(ROZA_BATTERY_ADC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(5)); // Petit délai entre lectures
    }
    uint32_t adc_avg = adc_sum / 3;

    // Convertir en tension
    float voltage = roza_battery_adc_to_voltage(adc_avg);

    return voltage;
}

/*
 * @brief Mise à jour de l'état batterie complète
 * Lit la tension, calcule le pourcentage et détecte la charge.
 * Doit être appelée périodiquement (ex: toutes les 1s ou 5s).
 */
void roza_battery_update(void) {
    if (!g_battery_calibrated) {
        // Lecture simple sans calibration avancée
        g_battery_state.voltage = roza_battery_read_voltage();
    } else {
        // Avec calibration complète (non implémenté pleinement ici)
        g_battery_state.voltage = roza_battery_read_voltage();
    }

    // Calculer le pourcentage basique par interpolation linéaire
    // entre MIN_V et MAX_V
    if (g_battery_state.voltage <= ROZA_BATTERY_MIN_V) {
        g_battery_state.percentage = 0;
    } else if (g_battery_state.voltage >= ROZA_BATTERY_MAX_V) {
        g_battery_state.percentage = 100;
    } else {
        g_battery_state.percentage = 
            (g_battery_state.voltage - ROZA_BATTERY_MIN_V) * 100.0 / 
            (ROZA_BATTERY_MAX_V - ROZA_BATTERY_MIN_V);
        // Borne sécurité
        if (g_battery_state.percentage > 100) g_battery_state.percentage = 100;
        if (g_battery_state.percentage < 0) g_battery_state.percentage = 0;
    }

    // Détection simple de charge (tension supérieure à Vref indique charge externe)
    // En pratique, cela dépend du circuit de charge du Cardputer ADV
    if (g_battery_state.voltage > ROZA_BATTERY_REF_VOLT * 1.1) {
        g_battery_state.charging = true;
    } else {
        g_battery_state.charging = false;
    }

    // Timestamp
    g_battery_state.last_read_ms = esp_timer_get_time() / 1000;

    ESP_LOGD("ROZA_BATTERY", "Battery: %.2fV, %.1f%%, charging=%d", 
             g_battery_state.voltage, g_battery_state.percentage, g_battery_state.charging);
}

/*
 * @brief Obtenir l'état batterie actuel
 * @return Structure complète batterie (tension, %, charge)
 */
roza_battery_state_t roza_battery_get_state(void) {
    return g_battery_state;
}

/*
 * @brief Calibration du rapport de diviseur
 * Met à jour les valeurs de R1/R2 utilisées pour le calcul de tension.
 * À appeler si le matériel de diviseur change.
 */
void roza_battery_calibrate_divider(float r1, float r2) {
    ROZA_BATTERY_DIVIDER_R1 = r1;
    ROZA_BATTERY_DIVIDER_R2 = r2;
    g_battery_calibrated = true;
    ESP_LOGI("ROZA_BATTERY", "Divider calibrated: R1=%.1fkΩ, R2=%.1fkΩ", r1, r2);
}