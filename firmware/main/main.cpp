#include "m5stack-cardputer.hpp"
#include "lvgl.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char TAG[] = "ROZA";

// Taille du buffer de dessin LVGL en nombre de pixels.
// Doit être suffisant pour une ligne complète + marge.
// Pour 240×135 en 16 bits : 240 * 2 octets = 480 octets par ligne.
// 20 lignes de buffer = ~96 Ko de RAM disponible (8 Mo Flash, pas de PSRAM).
// On utilise un buffer dynamique alloué au démarrage si besoin, ou la configuration par défaut de LVGL.
constexpr size_t pixel_buffer_size = espp::M5StackCardputer::lcd_width() * 20;

// Callback appelé à chaque événement clavier
void on_key(const espp::M5StackCardputer::KeyEvent &event) {
    // Ne traiter que les événements "appuyé"
    if (!event.pressed) {
        return;
    }

    // Valeurs ASCII imprimables (espace jusqu'à tilde)
    if (event.value >= 32 && event.value <= 126) {
        ESP_LOGI(TAG, "Touche : %c", static_cast<char>(event.value));
    } else {
        // Gestion des touches spéciales et modificateurs
        // Valeurs du TCA8418 Cardputer ADV :
        // - Enter : 0x0D (13)
        // - Escape/Back : gestionnée par le BSP ou valeur spécifique
        // - Flèches/Fn/Shift : valeurs selon le mapping matériel
        // On journalise la position et la valeur brute pour traiter plus loin
        ESP_LOGI(TAG,
                 "Touche spéciale : valeur=%d ligne=%u colonne=%u pressed=%d",
                 event.value,
                 static_cast<unsigned>(event.row),
                 static_cast<unsigned>(event.col),
                 event.pressed);
    }
}

void create_boot_screen() {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x172018), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "ROZA");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8DDBD), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -25);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle,
                      "TERMINAL AUTONOME\nCARDPUTER ADV\nBASE ESP-IDF / ESPP");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xC4B58E), LV_PART_MAIN);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 18);
} // namespace

extern "C" void app_main(void) {
    auto &cardputer = espp::M5StackCardputer::get();
    cardputer.set_log_level(espp::Logger::Verbosity::INFO);

    ESP_LOGI(TAG, "Démarrage de la fondation ROZA");

    // --- Initialisation LCD indépendante ---
    ESP_LOGI(TAG, "Initialisation de l'écran ST7789V2...");
    if (!cardputer.initialize_lcd()) {
        ESP_LOGE(TAG, "ÉCHEC : Initialisation de l'écran impossible");
        // On continue quand même ; l'affichage sera géré par LVGL plus tard
    } else {
        ESP_LOGI(TAG, "Écran ST7789V2 initialisé avec succès");
    }

    // Initialisation affichage LVGL avec buffer de pixels
    ESP_LOGI(TAG, "Initialisation de l'affichage LVGL (buffer %zu pixels)...",
             pixel_buffer_size);
    if (!cardputer.initialize_display(pixel_buffer_size)) {
        ESP_LOGE(TAG, "ÉCHEC : Initialisation de l'affichage LVGL impossible");
        // On continue quand même ; le boot screen tentera quand même de s'afficher
    } else {
        ESP_LOGI(TAG, "Affichage LVGL initialisé avec succès");
    }

    create_boot_screen();

    // --- Initialisation clavier indépendante ---
    ESP_LOGI(TAG, "Initialisation du clavier TCA8418...");
    if (!cardputer.initialize_keyboard(on_key)) {
        ESP_LOGE(TAG, "ÉCHEC : Initialisation du clavier impossible");
        // On continue quand même ; le clavier pourra être réinitialisé plus tard
    } else {
        ESP_LOGI(TAG, "Clavier TCA8418 initialisé avec succès");
    }

    // --- Initialisation IMU indépendante (variant ADV uniquement) ---
    if (cardputer.variant() == espp::M5StackCardputer::Variant::ADV) {
        ESP_LOGI(TAG, "Initialisation de l'IMU BMI270 (variant ADV)...");
        if (cardputer.initialize_imu()) {
            ESP_LOGI(TAG, "IMU BMI270 initialisée avec succès");
        } else {
            ESP_LOGW(TAG, "IMU BMI270 indisponible ou non détectée sur cette variante");
        }
    } else {
        ESP_LOGW(TAG, "Carte détectée différente du Cardputer ADV attendu ; pas d'IMU initialisée");
    }

    // Politique par défaut : Wi-Fi/Bluetooth/diffusion laissés inactifs
    ESP_LOGI(TAG, "Base démarrée ; réseau et émission radio laissés inactifs conformément à la politique ROZA");

    // Boucle principale : mise en veille active
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} // namespace