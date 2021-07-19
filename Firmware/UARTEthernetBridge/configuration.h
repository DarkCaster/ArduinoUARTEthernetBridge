#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//status LED
#ifdef ARDUINO_PRO_MINI_CONFIG
#define STATUS_LED 2
#elif ARDUINO_MEGA2560_CONFIG
#define STATUS_LED LED_BUILTIN
#endif

//ext serial config
#ifdef ARDUINO_PRO_MINI_CONFIG
#define UART_BUFFER_SIZE 64
#define UART_COUNT 1
#define UART_DEFS { &Serial }
#define UART_RX_PINS { 0 }
#define UART_RST_PINS { 3 }
#elif ARDUINO_MEGA2560_CONFIG
#define UART_BUFFER_SIZE 64
#define UART_COUNT 3
#define UART_DEFS { &Serial1, &Serial2, &Serial3 }
#define UART_RX_PINS { 19, 17, 15 }
#define UART_RST_PINS { 26, 24, 22 }
#endif

#define IDLE_POLL_INTERVAL_US 1000000

//params for ENC28J60 ethernet shield
#define ENC28J60_MACADDR { 0x00,0x16,0x3E,0x65,0xE3,0x66 }
#ifdef ARDUINO_PRO_MINI_CONFIG
#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 9
#elif ARDUINO_MEGA2560_CONFIG
#define PIN_SPI_ENC28J60_CS PIN_SPI_SS
#define PIN_ENC28J60_RST 49
#endif

//network params
#define NET_NAME "ENC28J65E366"
#define TCP_PORT 50000

//other params
#define RESET_TIME_MS 100
#define COLD_BOOT_WARMUP 1000
#ifdef ARDUINO_PRO_MINI_CONFIG
#define DATA_BUFFER_SIZE (UART_BUFFER_SIZE*6)
#elif ARDUINO_MEGA2560_CONFIG
#define DATA_BUFFER_SIZE (UART_BUFFER_SIZE*20)
#endif

//package format defines
#define PKG_HDR_SZ 2
#define CMD_HDR_SIZE 3
#define META_SZ (PKG_HDR_SZ+CMD_HDR_SIZE*UART_COUNT)
#define META_CRC_SZ 1
#define PACKAGE_SIZE (META_SZ+META_CRC_SZ+UART_BUFFER_SIZE*UART_COUNT) //seq number 2 bytes, (1byte cmd + 2bytes payload)*UART_COUNT, 1 byte crc, uart payload -> UART_BUFFER_SIZE*UART_COUNT

#endif
