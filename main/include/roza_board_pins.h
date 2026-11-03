#pragma once

/*
 * M5Stack Cardputer ADV + Cap LoRa-1262.
 * SPI SD and SX1262 share the same physical bus.
 * Do not instantiate two independent SPI hosts on these pins.
 */

#define ROZA_PIN_LCD_BL       38
#define ROZA_PIN_LCD_RST      33
#define ROZA_PIN_LCD_RS       34
#define ROZA_PIN_LCD_MOSI     35
#define ROZA_PIN_LCD_SCK      36
#define ROZA_PIN_LCD_CS       37

#define ROZA_PIN_I2C_SDA       8
#define ROZA_PIN_I2C_SCL       9
#define ROZA_PIN_KEY_INT      11
#define ROZA_PIN_BATTERY_ADC  10

#define ROZA_PIN_SPI_MOSI     14
#define ROZA_PIN_SPI_MISO     39
#define ROZA_PIN_SPI_SCK      40
#define ROZA_PIN_SD_CS        12
#define ROZA_PIN_LORA_CS       5

#define ROZA_PIN_LORA_RESET    3
#define ROZA_PIN_LORA_IRQ      4
#define ROZA_PIN_LORA_BUSY     6

#define ROZA_PIN_GNSS_RX      13
#define ROZA_PIN_GNSS_TX      15
#define ROZA_PIN_IR_TX        44
