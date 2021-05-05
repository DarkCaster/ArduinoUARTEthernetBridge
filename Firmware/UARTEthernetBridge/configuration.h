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
#define NET_PORT 50000
#define NET_NAME "ENC28J65E366"

//defs for requests and answers, do not change please
#define REQ_NONE 0x00 //never sent over line, used to notify that no data ready to process
#define REQ_EOF 0xFF //never sent over line, used to notify client disconnect
#define REQ_CONNECT 0x01
#define REQ_DISCONNECT 0x02
#define REQ_DATA 0x03
#define REQ_PING 0x04
#define REQ_WD 0xFE

#define ANS_DATA 0x03
#define ANS_PING 0x04

#define REQ_NONE_LEN 0
#define REQ_EOF 0
#define REQ_CONNECT_LEN (1+2+(3+1)*UART_COUNT) //1byte - cmd header, 2 - bytes - watchdog interval ms, (3byte - uart speed, 1byte - mode) * uart count
#define REQ_DISCONNECT_LEN 1
#define REQ_DATA_LEN (1+(1+UART_BUFFER_SIZE)*UART_COUNT)
#define REQ_PING_LEN (1+(1+UART_BUFFER_SIZE)*UART_COUNT) //same as data, to test total latency
#define REQ_WD_LEN 1 //ping watchdog

#define ANS_DATA_LEN (1+(1+UART_BUFFER_SIZE)*UART_COUNT)
#define AND_PING_LEN (1+(1+UART_BUFFER_SIZE)*UART_COUNT) //same as data, to test total latency

#endif
