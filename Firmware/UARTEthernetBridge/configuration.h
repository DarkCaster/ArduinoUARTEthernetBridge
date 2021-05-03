#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//debug serial port params, not used in regular communication
#define DEBUG_SERIAL_PORT Serial
#define DEBUG_SERIAL_PORT_SPEED 115200
#define DEBUG_SERIAL_RX_PIN 0
#define DEBUG_LED LED_BUILTIN

//ext serial config
#define UART_COUNT 3
#define UART_BUFFER_SIZE 64

//params for ENC28J60 ethernet shield
#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 49
#define ENC28J60_MACADDR { 0x00,0x16,0x3E,0x65,0xE3,0x66 }

//network params
#define UDP_PORT 50000
#define DEV_NAME "ENC28J65E366"

#endif
