#pragma once

/*
 * РОЗА — Gestionnaire de bus SPI partagé entre microSD et SX1262
 * Licence : Apache 2.0 (code ESP-IDF original) / adapted for ROZA
 * 
 * Ce module fournit un service mutex/SPI partagé pour arbitrer l'accès
 * au bus SPI partagé entre la microSD (CS=G12) et le module SX1262 (CS=G5).
 * 
 * Règle absolue : GPIO5 (SX1262 NSS) doit être mis à HIGH par défaut
 * pour éviter les conflits avec la microSD, comme documenté par Launcher
 * debug ("SD card mount Issue - GPIO5 interference").
 */

#pragma once

#include <stdint.h>
#include "soc/spi_struct.h"

// États du gestionnaire SPI partagé
typedef enum {
    ROZA_SPI_STATE_IDLE,        // Bus inactif, personne n'a le mutex
    ROZA_SPI_STATE_SD,          // MicroSD en cours d'accès
    ROZA_SPI_STATE_LORA,        // SX1262 en cours d'accès
    ROZA_SPI_STATE_ERROR        // Erreur d'accès ou timeout
} roza_spi_state_t;

// Structure de configuration SPI partagé
typedef struct {
    uint8_t sd_cs_pin;          // CS microSD (G12)
    uint8_t lora_cs_pin;        // CS SX1262 (G5)
    uint8_t sck_pin;            // SCK partagé (G40)
    uint8_t mosi_pin;           // MOSI partagé (G14)
    uint8_t miso_pin;           // MISO partagé (G39)
    uint32_t spi_host;          // Host SPI number (HSPI or VSPI)
    uint8_t mutex_timeout_ms;   // Timeout attente mutex (ms)
} roza_spi_config_t;

// Structure d'état du bus SPI
typedef struct {
    roza_spi_state_t current_state;
    bool mutex_held;
    uint32_t last_access_tick;
    uint8_t pending_request;    // 0=none, 1=SD, 2=LoRa
} roza_spi_bus_t;

/*
 * @brief Initialisation du bus SPI partagé
 * @param config Configuration des broches et paramètres
 * @return true si initialization réussie
 */
bool roza_spi_shared_init(const roza_spi_config_t *config);

/*
 * @brief Demander l'accès au bus SPI (blocage si occupé)
 * @param request_type 1=MicroSD, 2=SX1262/LoRa
 * @return true si accès obtenu, false si timeout
 */
bool roza_spi_shared_request(uint8_t request_type);

/*
 * @brief Libérer le bus SPI
 * @param request_type Type qui libère (doit correspondre à la demande)
 * @return true si libéré avec succès
 */
bool roza_spi_shared_release(uint8_t request_type);

/*
 * @brief Obtenir l'état actuel du bus
 * @return Structure d'état bus SPI
 */
roza_spi_state_t roza_spi_shared_get_state(void);

/*
 * @brief Verrouiller les broches CS (mettre CS en inactif/High)
 * À appeler après toute opération SPI pour désélectionner les deux périphériques.
 */
void roza_spi_shared_disable_cs(void);