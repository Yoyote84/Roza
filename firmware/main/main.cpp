#include "m5stack-cardputer.hpp"
#include "lvgl.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char TAG[] = "ROZA";

void on_key(const espp::M5StackCardputer::KeyEvent &event) {
    if (!event.pressed) {
        return;
    }

    if (event.value >= 32 && event.value <= 126) {
        ESP_LOGI(TAG, "Touche : %c", event.value);
    } else {
        ESP_LOGI(TAG, "Touche spéciale ou modificateur (ligne %u, colonne %u)",
                 static_cast<unsigned>(event.row),
                 static_cast<unsigned>(event.col));
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
}

} // namespace

extern "C" void app_main(void) {
    auto &cardputer = espp::M5StackCardputer::get();
    cardputer.set_log_level(espp::Logger::Verbosity::INFO);

    ESP_LOGI(TAG, "Démarrage de la fondation ROZA");

    if (!cardputer.initialize_lcd()) {
        ESP_LOGE(TAG, "Initialisation de l'écran impossible");
        return;
    }

    constexpr size_t pixel_buffer_size =
        espp::M5StackCardputer::lcd_width() * 20;

    if (!cardputer.initialize_display(pixel_buffer_size)) {
        ESP_LOGE(TAG, "Initialisation de l'affichage LVGL impossible");
        return;
    }

    create_boot_screen();

    if (!cardputer.initialize_keyboard(on_key)) {
        ESP_LOGE(TAG, "Initialisation du clavier impossible");
    }

    if (cardputer.variant() == espp::M5StackCardputer::Variant::ADV) {
        if (cardputer.initialize_imu()) {
            ESP_LOGI(TAG, "IMU BMI270 initialisée");
        } else {
            ESP_LOGW(TAG, "IMU BMI270 indisponible");
        }
    } else {
        ESP_LOGW(TAG, "Carte détectée différente du Cardputer ADV attendu");
    }

    // Aucun démarrage Wi-Fi/Bluetooth, aucun montage ou formatage SD,
    // aucune émission LoRa au démarrage.
    ESP_LOGI(TAG, "Base démarrée ; réseau et émission radio laissés inactifs");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
