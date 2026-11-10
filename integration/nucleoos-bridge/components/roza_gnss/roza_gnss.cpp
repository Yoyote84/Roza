#include "nucleo_app.h"
#include "app_gfx.h"
#include "launcher_theme.h"
#include "roza_gnss_nmea.h"

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

namespace {

static constexpr uart_port_t GNSS_UART = UART_NUM_1;
static constexpr int GNSS_TX_PIN = 13;
static constexpr int GNSS_RX_PIN = 15;
static constexpr int GNSS_BAUD = 115200;

static bool s_uart_installed;
static bool s_uart_ready;
static char s_error[48];
static char s_line[160];
static size_t s_line_length;
static roza_gnss_fix_t s_fix;

static void set_error(const char *message)
{
    snprintf(s_error, sizeof(s_error), "%s", message ? message : "Erreur UART");
}

static void gnss_enter()
{
    s_uart_installed = false;
    s_uart_ready = false;
    s_error[0] = '\0';
    s_line_length = 0;
    roza_gnss_fix_reset(&s_fix);

    const esp_err_t install = uart_driver_install(GNSS_UART, 2048, 0, 0, nullptr, 0);
    if (install != ESP_OK) {
        set_error(install == ESP_ERR_INVALID_STATE ? "UART1 deja utilise" : "UART1 indisponible");
        nucleo_app_set_hint("ESC retour");
        nucleo_app_request_draw();
        return;
    }
    s_uart_installed = true;

    uart_config_t config = {};
    config.baud_rate = GNSS_BAUD;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t result = uart_param_config(GNSS_UART, &config);
    if (result == ESP_OK) {
        // Cardputer ADV TX=G13 -> GNSS RX, Cardputer ADV RX=G15 <- GNSS TX.
        result = uart_set_pin(GNSS_UART, GNSS_TX_PIN, GNSS_RX_PIN,
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (result != ESP_OK) {
        set_error("Configuration GNSS impossible");
        uart_driver_delete(GNSS_UART);
        s_uart_installed = false;
        nucleo_app_set_hint("ESC retour");
        nucleo_app_request_draw();
        return;
    }

    s_uart_ready = true;
    nucleo_app_set_hint("r reinitialiser  ESC retour");
    nucleo_app_request_draw();
}

static void consume_byte(char value)
{
    if (value == '\r') return;
    if (value == '\n') {
        if (s_line_length > 0) {
            s_line[s_line_length] = '\0';
            if (roza_gnss_parse_sentence(s_line, &s_fix)) {
                nucleo_app_request_draw();
            }
        }
        s_line_length = 0;
        return;
    }

    if (s_line_length + 1 >= sizeof(s_line)) {
        s_line_length = 0;
        return;
    }
    s_line[s_line_length++] = value;
}

static void gnss_tick()
{
    if (!s_uart_ready) return;

    uint8_t bytes[128];
    const int count = uart_read_bytes(GNSS_UART, bytes, sizeof(bytes), 0);
    for (int i = 0; i < count; ++i) consume_byte(static_cast<char>(bytes[i]));
}

static void draw_row(int y, const char *label, const char *value)
{
    d.setTextColor(MUTED, BG);
    d.setCursor(8, y);
    d.print(label);
    d.setTextColor(FG, BG);
    d.setCursor(92, y);
    d.print(value);
}

static void gnss_draw()
{
    const int top = nucleo_app_content_top();
    const int height = nucleo_app_content_height();
    d.setFont(&fonts::Font0);
    d.setTextSize(1);
    d.fillRect(0, top, W, height, BG);

    d.setTextColor(C_GREEN, BG);
    d.setCursor(8, top + 1);
    d.print("POSITION GNSS");
    d.drawFastHLine(8, top + 13, W - 16, LINE);

    if (!s_uart_ready) {
        d.setTextColor(C_RED, BG);
        d.setCursor(8, top + 25);
        d.print(s_error[0] ? s_error : "GNSS non disponible");
        return;
    }

    if (!s_fix.has_fix) {
        d.setTextColor(C_BLUE, BG);
        d.setCursor(8, top + 25);
        d.print("Recherche de satellites...");
        d.setTextColor(MUTED, BG);
        d.setCursor(8, top + 42);
        d.print("Cap LoRa-1262 / GPS 115200");
        char sats[24];
        snprintf(sats, sizeof(sats), "Satellites : %d", s_fix.satellites);
        d.setCursor(8, top + 59);
        d.print(sats);
        return;
    }

    char value[40];
    snprintf(value, sizeof(value), "%.5f", s_fix.latitude_deg);
    draw_row(top + 18, "Latitude", value);
    snprintf(value, sizeof(value), "%.5f", s_fix.longitude_deg);
    draw_row(top + 32, "Longitude", value);
    snprintf(value, sizeof(value), "%.1f m", s_fix.altitude_m);
    draw_row(top + 46, "Altitude", value);
    snprintf(value, sizeof(value), "%d  HDOP %.1f", s_fix.satellites, s_fix.hdop);
    draw_row(top + 60, "Satellites", value);
    snprintf(value, sizeof(value), "%.1f km/h", s_fix.speed_kmh);
    draw_row(top + 74, "Vitesse", value);
    snprintf(value, sizeof(value), "%.6s  %.6s", s_fix.utc_time, s_fix.utc_date);
    draw_row(top + 88, "UTC", value);
}

static void gnss_key(int key, char ch)
{
    if (key == NK_CHAR && (ch == 'r' || ch == 'R')) {
        roza_gnss_fix_reset(&s_fix);
        s_line_length = 0;
        nucleo_app_request_draw();
    }
}

static void gnss_exit()
{
    if (s_uart_installed) uart_driver_delete(GNSS_UART);
    s_uart_installed = false;
    s_uart_ready = false;
    s_line_length = 0;
    s_error[0] = '\0';
}

} // namespace

extern "C" void roza_register_gnss(void)
{
    static const nucleo_app_def_t app = {
        "roza-gnss",
        "Position GNSS",
        "Connect",
        "Position GPS via Cap LoRa-1262",
        'G',
        C_GREEN,
        gnss_enter,
        gnss_key,
        gnss_tick,
        gnss_draw,
        gnss_exit,
        0,
        nullptr,
        nullptr
    };
    nucleo_app_register(&app);
}
