#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//debug serial port params, not used in regular communication
#define DEBUG_SERIAL_PORT Serial
#define DEBUG_SERIAL_PORT_SPEED 115200
#define DEBUG_SERIAL_RX_PIN 0
#define DEBUG_LED LED_BUILTIN

//ext serial config
#define UART_BUFFER_SIZE 64
#define UART_COUNT 3
#define UART_DEFS { &Serial1, &Serial2, &Serial3 }
#define UART_RX_PINS { 19, 17, 15 }
#define UART_MAX_POLL_INTERVAL_MS 5

//params for ENC28J60 ethernet shield
#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 49
#define ENC28J60_MACADDR { 0x00,0x16,0x3E,0x65,0xE3,0x66 }

//network params
#define NET_PORTS { 50000, 50001, 50002 }
#define NET_NAME "ENC28J65E366"

//other params
#define DEFAULT_WD_INTERVAL 1000
#define BACK_BUFFER_SEGMENTS 5

#endif
