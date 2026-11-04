#include "roza_spi_shared.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_system.h>

// Configuration par défaut pour Cardputer ADV
static roza_spi_config_t g_spi_config = {
    .sd_cs_pin    = 12,
    .lora_cs_pin  = 5,
    .sck_pin      = 40,
    .mosi_pin     = 14,
    .miso_pin     = 39,
    .spi_host     = 1,  // SPI1 (HSPI) sur ESP32-S3
    .mutex_timeout_ms = 1000
};

// Instance statique du bus SPI
static roza_spi_bus_t g_spi_bus = {
    .current_state = ROZA_SPI_STATE_IDLE,
    .mutex_held = false,
    .last_access_tick = 0,
    .pending_request = 0
};

// Mutex logiciel pour l'accès au bus SPI
static portMUTEX_TYPE g_spi_mutex;

// Initialisation du mutex au premier appel
static void ensure_mutex(void) {
    if (g_spi_mutex == NULL) {
        portMUTEX_INIT(g_spi_mutex);
    }
}

/*
 * @brief Initialisation du bus SPI partagé
 * Configure les broches CS, le mutex logiciel, et prépare l'environnement.
 * Note importante : GPIO5 (LORA_CS) doit être mis à HIGH par défaut
 * pour éviter les conflits avec la microSD.
 */
bool roza_spi_shared_init(const roza_spi_config_t *config) {
    if (config != NULL) {
        g_spi_config = *config;
    }

    // Initialiser le mutex logiciel
    ensure_mutex();

    // Configurer les broches CS en sortie, initialement HIGH (inactif)
    // C'est CRITICAL : éviter les conflits SD/Lora au démarrage
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << g_spi_config.sd_cs_pin) | (1ULL << g_spi_config.lora_cs_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Mettre CS par défaut à HIGH (désactivé)
    gpio_set_level(g_spi_config.sd_cs_pin, 1);
    gpio_set_level(g_spi_config.lora_cs_pin, 1);

    // Configurer SCK, MOSI, MISO en mode fonctionnel
    // (Ces broches sont partagées, on les configures ici pour référence)
    gpio_set_direction(g_spi_config.sck_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(g_spi_config.mosi_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(g_spi_config.miso_pin, GPIO_MODE_INPUT);

    ESP_LOGI("ROZA_SPI_SHARED", "SPI shared bus initialized:");
    ESP_LOGI("ROZA_SPI_SHARED", "  SD_CS=GPIO%d, LoRA_CS=GPIO%d", g_spi_config.sd_cs_pin, g_spi_config.lora_cs_pin);
    ESP_LOGI("ROZA_SPI_SHARED", "  SCK=GPIO%d, MOSI=GPIO%d, MISO=GPIO%d",
             g_spi_config.sck_pin, g_spi_config.mosi_pin, g_spi_config.miso_pin);

    // Démarrer dans état IDLE avec CS désactivés
    g_spi_bus.current_state = ROZA_SPI_STATE_IDLE;
    g_spi_bus.mutex_held = false;

    return true;
}

/*
 * @brief Demander l'accès au bus SPI (blocage si occupé)
 * @param request_type 1=MicroSD, 2=SX1262/LoRa
 * @return true si accès obtenu, false si timeout
 */
bool roza_spi_shared_request(uint8_t request_type) {
    if (g_spi_bus.mutex_held) {
        // Bus déjà occupé
        ESP_LOGW("ROZA_SPI_SHARED", "Bus already in use, request_type=%d", request_type);
        return false;
    }

    // Prendre le mutex logiciel
    portENTER_CRITICAL(&g_spi_mutex);

    g_spi_bus.mutex_held = true;
    g_spi_bus.current_state = (request_type == 1) ? ROZA_SPI_STATE_SD : ROZA_SPI_STATE_LORA;
    g_spi_bus.pending_request = request_type;
    g_spi_bus.last_access_tick = esp_timer_get_time() / 1000;

    ESP_LOGD("ROZA_SPI_SHARED", "SPI request granted: type=%d, state=%s",
             request_type, g_spi_bus.current_state == ROZA_SPI_STATE_SD ? "SD" : "LoRA");

    return true;
}

/*
 * @brief Libérer le bus SPI
 * @param request_type Type qui libère (doit correspondre à la demande en cours)
 * @return true si libéré avec succès
 */
bool roza_spi_shared_release(uint8_t request_type) {
    if (!g_spi_bus.mutex_held) {
        ESP_LOGW("ROZA_SPI_SHARED", "Bus not held, cannot release");
        return false;
    }

    // Vérifier que le type à libérer correspond à la demande actuelle
    if (g_spi_bus.pending_request != request_type) {
        ESP_LOGW("ROZA_SPI_SHARED", "Release type mismatch: held=%d, release=%d",
                 g_spi_bus.pending_request, request_type);
        // Tout de même libérer
    }

    // Release the mutex
    portEXIT_CRITICAL(&g_spi_mutex);

    g_spi_bus.mutex_held = false;
    g_spi_bus.current_state = ROZA_SPI_STATE_IDLE;
    g_spi_bus.pending_request = 0;

    // S'assurer que les deux CS sont mises à HIGH (inactif)
    gpio_set_level(g_spi_config.sd_cs_pin, 1);
    gpio_set_level(g_spi_config.lora_cs_pin, 1);

    ESP_LOGD("ROZA_SPI_SHARED", "SPI released, state=IDLE");

    return true;
}

/*
 * @brief Obtenir l'état actuel du bus
 * @return État actuel du bus SPI
 */
roza_spi_state_t roza_spi_shared_get_state(void) {
    return g_spi_bus.current_state;
}

/*
 * @brief Verrouiller les broches CS (mettre CS en inactif/High)
 * Cette fonction s'assure que les deux périphériques CS sont désactivés.
 * À appeler après toute opération SPI pour éviter les conflits parasites.
 */
void roza_spi_shared_disable_cs(void) {
    // Mettre les deux CS à HIGH (inactif)
    gpio_set_level(g_spi_config.sd_cs_pin, 1);
    gpio_set_level(g_spi_config.lora_cs_pin, 1);

    // Optionnel : forcer l'état en IDLE si pas déjà fait
    if (g_spi_bus.current_state != ROZA_SPI_STATE_IDLE) {
        g_spi_bus.current_state = ROZA_SPI_STATE_IDLE;
    }
}