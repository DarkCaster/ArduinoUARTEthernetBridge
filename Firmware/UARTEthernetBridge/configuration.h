#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//persistent settings (NOTE: may be removed in future)
#define EEPROM_SETTINGS_ADDR 0x0000
#define EEPROM_SETTINGS_LEN 0x0040

//debug serial port params, not used in regular communication
#define DEBUG_SERIAL_PORT Serial
#define DEBUG_SERIAL_PORT_SPEED 115200
#define DEBUG_SERIAL_RX_PIN 0
#define DEBUG_LED LED_BUILTIN

//params for ENC28J60 ethernet shield
#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 49
#define ENC28J60_MACADDR { 0x00,0x16,0x3E,0x65,0xE3,0x66 }
#define TCP_PORT 50000


#define COMMAND_BUFFER_SIZE 128
#define RESPONSE_BUFFER_SIZE 128
#define CMDRSP_BUFF_TYPE uint8_t

#endif
