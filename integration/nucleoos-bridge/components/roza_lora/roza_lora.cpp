#include "nucleo_app.h"
#include "nucleo_kbd.h"
#include "app_gfx.h"
#include "launcher_theme.h"

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_err.h"
#include <RadioLib.h>
#include <hal/ESP-IDF/EspHal.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {
static constexpr int PIN_SCK=40, PIN_MISO=39, PIN_MOSI=14;
static constexpr int PIN_NSS=5, PIN_DIO1=4, PIN_RESET=3, PIN_BUSY=6;
static constexpr uint8_t IOE_ADDR=0x43;
static constexpr float FREQUENCY_MHZ=868.0f;
static constexpr float BANDWIDTH_KHZ=125.0f;
static constexpr uint8_t SF=12, CR=5, SYNC_WORD=0x34;
static constexpr int8_t TX_POWER_DBM=10;
static constexpr uint16_t PREAMBLE=20;
static constexpr float TCXO_VOLTAGE=3.0f;

// NucleoOS owns SPI2 for the microSD. RadioLib attaches another SPI device to
// that already-initialized host; it never initializes a second bus.
static EspHal s_hal(PIN_SCK, PIN_MISO, PIN_MOSI, SPI2_HOST, 2000000U);
static Module s_module(&s_hal, PIN_NSS, PIN_DIO1, PIN_RESET, PIN_BUSY);
static SX1262 s_radio(&s_module);
static i2c_master_dev_handle_t s_ioe=nullptr;
static bool s_initialized=false, s_receiving=false;
static volatile bool s_packet_pending=false;
static char s_error[56]="";
static char s_packet[96]="Aucun paquet recu";
static size_t s_packet_size=0;
static float s_rssi=0.0f, s_snr=0.0f;

static esp_err_t read_reg(uint8_t reg, uint8_t *value) {
    if (!s_ioe || !value) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(s_ioe, &reg, 1, value, 1, 100);
}
static esp_err_t write_reg(uint8_t reg, uint8_t value) {
    if (!s_ioe) return ESP_ERR_INVALID_STATE;
    uint8_t data[2]={reg,value};
    return i2c_master_transmit(s_ioe, data, sizeof(data), 100);
}
static bool update_reg(uint8_t reg, uint8_t set_bits, uint8_t clear_bits) {
    uint8_t value=0;
    if (read_reg(reg,&value)!=ESP_OK) return false;
    value=(uint8_t)((value|set_bits)&(uint8_t)~clear_bits);
    return write_reg(reg,value)==ESP_OK;
}
static bool enable_antenna_switch() {
    // Reuse the ADV I2C bus initialized by the TCA8418 keyboard driver.
    auto bus=static_cast<i2c_master_bus_handle_t>(nucleo_kbd_i2c_bus());
    if (!bus) { snprintf(s_error,sizeof(s_error),"Bus I2C ADV indisponible"); return false; }
    if (!s_ioe) {
        i2c_device_config_t cfg={};
        cfg.dev_addr_length=I2C_ADDR_BIT_LEN_7;
        cfg.device_address=IOE_ADDR;
        cfg.scl_speed_hz=400000;
        const esp_err_t e=i2c_master_bus_add_device(bus,&cfg,&s_ioe);
        if (e!=ESP_OK) { s_ioe=nullptr; snprintf(s_error,sizeof(s_error),"Expandeur I2C: %d",(int)e); return false; }
    }
    uint8_t id=0;
    if (read_reg(0x01,&id)!=ESP_OK || (id & 0xE0) != 0xA0) {
        snprintf(s_error,sizeof(s_error),"Expandeur PI4IOE5V6408 absent"); return false;
    }
    // Load HIGH before enabling P0 as an output to avoid a low pulse on SX_ANT_SW.
    // P0: output direction=1, high-impedance=0, output state=1.
    if (!update_reg(0x05,0x01,0x00) ||
        !update_reg(0x03,0x01,0x00) ||
        !update_reg(0x07,0x00,0x01)) {
        snprintf(s_error,sizeof(s_error),"Commutateur antenne RF indisponible"); return false;
    }
    return true;
}
static void IRAM_ATTR packet_received_isr() { s_packet_pending=true; }

static bool listen_start() {
    s_packet_pending=false;
    const int16_t result=s_radio.startReceive();
    if (result!=RADIOLIB_ERR_NONE) {
        snprintf(s_error,sizeof(s_error),"Reception SX1262: %d",result);
        s_receiving=false; return false;
    }
    s_error[0]='\0'; s_receiving=true; return true;
}
static void radio_enter() {
    s_error[0]='\0'; s_packet_pending=false; s_receiving=false;
    s_packet_size=0; snprintf(s_packet,sizeof(s_packet),"Aucun paquet recu");
    if (!enable_antenna_switch()) {
        nucleo_app_set_hint("Cap LoRa-1262 requis  |  ESC retour");
        nucleo_app_request_draw(); return;
    }
    if (!s_initialized) {
        const int16_t result=s_radio.begin(FREQUENCY_MHZ,BANDWIDTH_KHZ,SF,CR,
            SYNC_WORD,TX_POWER_DBM,PREAMBLE,TCXO_VOLTAGE,true);
        if (result!=RADIOLIB_ERR_NONE) {
            snprintf(s_error,sizeof(s_error),"Initialisation SX1262: %d",result);
            nucleo_app_set_hint("Verifier le Cap et son antenne  |  ESC retour");
            nucleo_app_request_draw(); return;
        }
        s_initialized=true;
    }
    s_radio.setPacketReceivedAction(packet_received_isr);
    nucleo_app_set_hint(listen_start() ? "r ecouter / arreter  |  ESC retour" : "Reception impossible  |  ESC retour");
    nucleo_app_request_draw();
}
static void read_packet() {
    s_packet_pending=false;
    const size_t len=s_radio.getPacketLength();
    if (!len || len>255) {
        snprintf(s_packet,sizeof(s_packet),"Paquet vide ou trop long"); s_packet_size=len;
    } else {
        uint8_t bytes[255]={};
        const int16_t result=s_radio.readData(bytes,len);
        if (result==RADIOLIB_ERR_NONE) {
            const size_t out=len<sizeof(s_packet)-1?len:sizeof(s_packet)-1;
            for (size_t i=0;i<out;i++) {
                const unsigned char c=bytes[i];
                s_packet[i]=(isprint(c)||c==' '||c=='\t')?(char)c:'.';
            }
            s_packet[out]='\0'; s_packet_size=len;
            s_rssi=s_radio.getRSSI(); s_snr=s_radio.getSNR();
        } else {
            snprintf(s_packet,sizeof(s_packet),"Lecture paquet: %d",result); s_packet_size=0;
        }
    }
    if (s_receiving) listen_start();
    nucleo_app_request_draw();
}
static void radio_tick() {
    if (s_initialized && s_receiving && s_packet_pending) read_packet();
}
static void radio_key(int key,char ch) {
    if (key!=NK_CHAR) return;
    if (ch=='r'||ch=='R') {
        if (!s_initialized) { radio_enter(); return; }
        if (s_receiving) {
            s_radio.standby(); s_receiving=false;
            nucleo_app_set_hint("Reception arretee  |  r reprendre  |  ESC retour");
        } else {
            nucleo_app_set_hint(listen_start()?"Reception active  |  r arreter  |  ESC retour":"Reception impossible  |  ESC retour");
        }
        nucleo_app_request_draw();
    }
}
static void radio_draw() {
    const int top=nucleo_app_content_top(), height=nucleo_app_content_height();
    d.setFont(&fonts::Font0); d.setTextSize(1); d.fillRect(0,top,W,height,BG);
    d.setTextColor(C_BLUE,BG); d.setCursor(8,top+1); d.print("LORA 868 / SX1262");
    d.drawFastHLine(8,top+12,W-16,LINE);

    d.setTextColor(FG,BG); d.setCursor(8,top+16); d.print("Frequence : 868.0 MHz");
    d.setCursor(8,top+28);
    d.print(s_receiving?"Reception active":(s_initialized?"En veille":"Indisponible"));

    char line[48];
    d.setTextColor(MUTED,BG);
    snprintf(line,sizeof(line),"Paquet : %u octets",(unsigned)s_packet_size);
    d.setCursor(8,top+40); d.print(line);
    snprintf(line,sizeof(line),"RSSI %.1f dBm  SNR %.1f dB",s_rssi,s_snr);
    d.setCursor(8,top+52); d.print(line);

    d.setTextColor(FG,BG); d.setCursor(8,top+64); d.print("Dernier paquet :");
    d.setTextColor(MUTED,BG); d.setCursor(8,top+76);
    char preview[40]; snprintf(preview,sizeof(preview),"%.37s",s_packet); d.print(preview);

    if (s_error[0]) {
        d.setTextColor(C_RED,BG); d.setCursor(8,top+height-24); d.print(s_error);
    }
    d.setTextColor(C_RED,BG); d.setCursor(8,top+height-12);
    d.print("Reception seule - emission desactivee");
}
static void radio_exit() {
    if (s_initialized) {
        s_radio.clearPacketReceivedAction();
        s_radio.standby(); s_radio.sleep();
    }
    s_receiving=false; s_packet_pending=false;
}
} // namespace

extern "C" void roza_register_lora(void) {
    static const nucleo_app_def_t app={
        "roza-lora","Reception LoRa","Connect",
        "Reception radio SX1262 via Cap LoRa-1262",'L',C_BLUE,
        radio_enter,radio_key,radio_tick,radio_draw,radio_exit,0,nullptr,nullptr
    };
    nucleo_app_register(&app);
}
