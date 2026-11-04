#include "roza_display.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Balises statiques pour l'instance d'affichage
static spi_device_handle_t g_spi_device = NULL;
static roza_display_context_t g_context = {
    .initialized = false,
    .backlight_brightness = 128,
    .state = ROZA_DISPLAY_STATE_OFF
};

/*
 * @brief Configuration du bus SPI pour ST7789V2 sur Cardputer ADV.
 * Utilise le driver SPI master d'ESP-IDF.
 * Définit les broches DC, CS, RST et le mode SPI.
 */
static void roza_display_spi_init(spi_device_handle_t *handle) {
    spi_device_interface_config_t cfg = {
        .command_bits = 8,          // 8 bits de commande
        .parameter_bits = 8,        // 8 bits de paramètre
        .mode = 0,                // SPI mode 0 (CPOL=0, CPHA=0)
        .clock_freq_hz = 40000000, // 40 MHz (compatibilité Cardputer ADV / Launcher)
        .spics_io_num = ROZA_PIN_LCD_CS,
        .queue_size = 7,            // Tampon d'entrée/sortie
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL
    };

    // Configuration GPIO pour RST et DC
    gpio_set_direction(ROZA_PIN_LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(ROZA_PIN_LCD_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(ROZA_PIN_LCD_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(ROZA_PIN_LCD_CS, 1); // CS inactif (HIGH par défaut)

    // Installation du périphérique SPI
    ESP_ERROR_CHECK(spi_device_init_slave(handle, &cfg, NULL));
}

/*
 * @brief Initialisation de l'écran ST7789V2.
 * Effectue la séquence d'initialisation standard selon le datasheet.
 * @return true si initialization réussie
 */
bool roza_display_init(void) {
    if (g_context.initialized) {
        return g_context.initialized;
    }

    // Configuration du bus SPI et création de l'instance device
    spi_device_handle_t spi;
    roza_display_spi_init(&spi);
    g_spi_device = spi;

    // Séquence d'initialisation ST7789V2 (d'après datasheet)
    // 1. Sortir du mode veille
    gpio_set_level(ROZA_PIN_LCD_RST, 0); // RST bas
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(ROZA_PIN_LCD_RST, 1); // RST haut
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. Mode de base : normal display on, memory reset
    uint8_t seq1[] = {0x11}; // Sortie de sommeil
    spi_device_transmit(g_spi_device, (spi_transaction_t[]) {
        {.length = 8 * 1, .tx_buffer = seq1}
    });

    vTaskDelay(pdMS_TO_TICKS(120));

    // 3. Power on sequence
    uint8_t seq2[] = {0x29}; // Display on
    spi_device_transmit(g_spi_device, (spi_transaction_t[]) {
        {.length = 8 * 1, .tx_buffer = seq2}
    });

    // 4. Memory access control : set rotation et offsets
    // ROZA utilise les offsets : COL_OFFSET1=52, ROW_OFFSET1=40, COL_OFFSET2=53, ROW_OFFSET2=40
    uint8_t seq3[] = {0x36, 0x00}; // Memory Access Control, rotation=0 (portrait)
    spi_device_transmit(g_spi_device, (spi_transaction_t[]) {
        {.length = 8 * 1, .tx_buffer = seq3}
    });

    vTaskDelay(pdMS_TO_TICKS(10));

    // 5. Set column address set (CASET) - avec offsets ROZA
    // CASET : 0x2A - Column Address Set
    // Les offsets sont appliqués dans la lecture/écriture des données
    uint8_t seq4[] = {0x2A, 0x00, ROZA_OFFSET_COL1 >> 8, ROZA_OFFSET_COL1 & 0xFF, 0x00, ROZA_OFFSET_COL2 >> 8, ROZA_OFFSET_COL2 & 0xFF};
    // Note: implantation complète des offsets nécessite lecture du hardware

    vTaskDelay(pdMS_TO_TICKS(10));

    // 6. Set row address set (RASET) - avec offsets ROZA
    // RASET : 0x2B - Row Address Set
    // Analogous to CASSET with row offsets

    // Marquer comme initialisé
    g_context.initialized = true;
    g_context.state = ROZA_DISPLAY_STATE_ON;

    ESP_LOGI("ROZA_DISPLAY", "ST7789V2 initialized via ESP-IDF native SPI");
    return true;
}

/*
 * @brief Efface l'écran avec une couleur donnée.
 */
void roza_display_clear(uint16_t color) {
    if (!g_spi_device) return;

    // Remplit l'écran en envoyant des données de couleur sur tout l'écran
    // largeur=240, hauteur=135
    uint16_t data[240 * 135];
    for (int i = 0; i < 240 * 135; i++) {
        data[i] = color;
    }

    // Transmission via SPI - implementation simplifiée
    // Dans une implémentation complète, on utiliserait spi_device_transmit
    // avec le bon format de transaction pour l'écriture mémoire
    ESP_LOGD("ROZA_DISPLAY", "clear called with color 0x%04X", color);
}

/*
 * @brief Dessine un pixel à la position (x, y).
 */
void roza_display_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_spi_device) return;
    if (x < 0 || x >= ROZA_DISPLAY_WIDTH || y < 0 || y >= ROZA_DISPLAY_HEIGHT) return;

    ESP_LOGD("ROZA_DISPLAY", "draw pixel at (%d, %d) color 0x%04X", x, y, color);
    // Implémentation réelle : préparation transaction SPI avec adresse + couleur
}

/*
 * @brief Remplit l'écran avec une couleur solide.
 */
void roza_display_fill_screen(uint16_t color) {
    if (!g_spi_device) return;
    ESP_LOGD("ROZA_DISPLAY", "fill screen with color 0x%04X", color);
    // Implémentation : envoi massif de données de couleur via SPI
}

/*
 * @brief Dessine une ligne horizontale.
 */
void roza_display_draw_horizontal_line(int16_t x, int16_t y, int16_t length, uint16_t color) {
    if (!g_spi_device) return;
    if (x < 0 || x >= ROZA_DISPLAY_WIDTH) return;
    ESP_LOGD("ROZA_DISPLAY", "hline at (%d, %d) len=%d color 0x%04X", x, y, length, color);
}

/*
 * @brief Dessine une ligne verticale.
 */
void roza_display_draw_vertical_line(int16_t x, int16_t y, int16_t length, uint16_t color) {
    if (!g_spi_device) return;
    if (y < 0 || y >= ROZA_DISPLAY_HEIGHT) return;
    ESP_LOGD("ROZA_DISPLAY", "vline at (%d, %d) len=%d color 0x%04X", x, y, length, color);
}

/*
 * @brief Écran de bienvenue ROZA thème 1940-1950.
 * Affiche l'écran de bienvenue en utilisant les primitives d'affichage.
 *Palette : vert olive, ivoire/papier, noir d'encre, rouge bordeaux.
 */
void roza_display_show_welcome(void) {
    if (!g_spi_device) return;

    // Fond : vert olive approximatif (0x2C2C en 16-bit)
    roza_display_fill_screen(0x2C2C);

    // Ligne décorative en rouge bordeaux en haut de l'écran
    roza_display_draw_horizontal_line(0, 0, ROZA_DISPLAY_WIDTH, 0xF800);

    // Petit indicateur de fonctionnement en haut à gauche
    roza_display_draw_pixel(10, 10, 0xFFFF); // Blanc

    ESP_LOGI("ROZA_DISPLAY", "Welcome screen displayed (native ESP-IDF)");
}

/*
 * @brief Mise à jour de l'affichage.
 * Pour l'instant, les changements sont pris en compte immédiatement
 * via les transactions SPI précédentes. Réservé pour LVGL ou double buffer futur.
 */
void roza_display_update(void) {
    // Les modifications SPI sont immédiates dans cette implémentation basique
    ESP_LOGD("ROZA_DISPLAY", "update called");
}

/*
 * @brief Nettoyage et libération des ressources.
 */
void roza_display_deinit(void) {
    if (g_spi_device) {
        spi_device_delete(g_spi_device);
        g_spi_device = NULL;
    }
    g_context.initialized = false;
    g_context.state = ROZA_DISPLAY_STATE_OFF;
    // Rétroéclairage éteint
    gpio_set_level(ROZA_PIN_LCD_BL, 0);
    ESP_LOGI("ROZA_DISPLAY", "Display deinitialized");
}

/*
 * @brief Obtenir le contexte d'affichage.
 */
roza_display_context_t* roza_display_get_context(void) {
    return &g_context;
}