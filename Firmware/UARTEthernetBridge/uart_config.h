#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include <Arduino.h>

#define CFG_RESET_REQUESTED(cfg) ((cfg.flags&0x1)!=0)
#define CFG_TEST_MODE(cfg) ((cfg.flags&0x2)!=0)

struct UARTConfig
{
    unsigned long speed;
    uint8_t mode;
    uint8_t flags;
    uint8_t collectIntMS;
};

#endif // UART_CONFIG_H
