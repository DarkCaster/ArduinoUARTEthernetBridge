# UART-to-Ethernet bridge with Arduino

Transfer data from UART via wired Ethernet/IPv4 network. Main goal of this project is to make cheap and stable solution for transfering UART data over Ethernet with delays small enough for interactive use (mainly for driving Klipper 3D-printer firmware for dev and debug purposes).

The system consists of two parts: remote board and client utility.

Remote board is physically connected to the target UART ports and currently use Arduino Mega 2560 compatible dev-board with atmega 2560 MCU and ENC28j60 ethernet SPI adapter. For now firmware is suited for Arduino Mega 2560 and it can drive its' 3 external UART ports simultaneously with peak throughput up to ~57 kbit/s (in both directions, full duplex) and loopback latency 5-15 ms (with block size 50b). If using Arduino Pro Mini with Atmega 328p MCU (currently experimental and not fully supported), it can reach throughput of ~250 kbit/s for it's single uart port and loopback latency 5-10 ms (with block size 128b).

Client utility is meant to be run on local PC with Linux. It simulate local serial port by using pseudoterminals (PTY) or TCP socket. It should be used as substitute to ttyUSB converter for applications to communicate with the remote side.

_NOTE: for now this project is highly experimental and may be removed in future, do not rely any of your work on it_

## TODO

- more verbose description
- build instructions and hw schematics
- usage examples
- rewrite serial comm. logic from Arduino implementation to custom to reduce load to MCU and improve performance
- port firmware to faster MCU's like STM32 or rp2040
