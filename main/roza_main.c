#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "roza_version.h"
#include "roza_board_pins.h"

static const char *TAG = "ROZA";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "%s — firmware autonome", ROZA_NAME);
    ESP_LOGI(TAG, "Version : %s", ROZA_VERSION);
    ESP_LOGI(TAG, "Langue  : %s", ROZA_LANGUAGE);
    ESP_LOGI(TAG, "Etape   : %s", ROZA_BUILD_STAGE);
    ESP_LOGI(TAG, "Cible   : ESP32-S3 / Cardputer ADV");
    ESP_LOGI(TAG, "Wi-Fi   : non initialise (desactive par defaut)");
    ESP_LOGI(TAG, "========================================");

    /*
     * G0: safe boot-only foundation.
     * No GPIO, SD, SPI, I2C, GNSS, SX1262 or display driver is started here.
     * Hardware drivers will be added behind the BSP and tested individually.
     */
    for (;;) {
        ESP_LOGI(TAG, "РОЗА : noyau de fondation actif");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
