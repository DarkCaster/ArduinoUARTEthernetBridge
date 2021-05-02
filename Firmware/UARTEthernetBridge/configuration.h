#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define EEPROM_SETTINGS_ADDR 0x0000
#define EEPROM_SETTINGS_LEN 0x0040

#define SERIAL_PORT Serial
#define SERIAL_PORT_SPEED 115200
#define SERIAL_RX_PIN 0
#define LED_DEBUG LED_BUILTIN

#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 49
#define ENC28J60_MACADDR { 0x00,0x16,0x3E,0x65,0xE3,0x66 }
#define TCP_PORT 50000

#define COMMAND_BUFFER_SIZE 128
#define RESPONSE_BUFFER_SIZE 128
#define CMDRSP_BUFF_TYPE uint8_t

#define DEBUG_SERIAL_PORT SERIAL_PORT

#endif
