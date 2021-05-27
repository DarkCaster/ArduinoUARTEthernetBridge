# UART-to-Ethernet bridge with Arduino

[![deepcode](https://www.deepcode.ai/api/gh/badge?key=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybTEiOiJnaCIsIm93bmVyMSI6IkRhcmtDYXN0ZXIiLCJyZXBvMSI6IkFyZHVpbm9VQVJURXRoZXJuZXRCcmlkZ2UiLCJpbmNsdWRlTGludCI6ZmFsc2UsImF1dGhvcklkIjoxNTI4NCwiaWF0IjoxNjIxMjk4NTM1fQ.iTItDY0wZAUOtIgfGGCs0iDc4g9sJe3lvN5z37t85PI)](https://www.deepcode.ai/app/gh/DarkCaster/ArduinoUARTEthernetBridge/_/dashboard?utm_content=gh%2FDarkCaster%2FArduinoUARTEthernetBridge)

Transfer data from UART via Ethernet/TCP-IP network. Main goal of this project is to make cheap and stable solution for transfering UART data over Ethernet with delays small enough for interactive usage (like driving Klipper 3D-printer firmware).

Remote part (connected to the target UART ports) currently using AVR-arduino compatible dev-bord and ENC28j60 ethernet SPI adapter. For now firmware suited for Arduino Mega 2560, and can drive all its' 3 HW UART ports simultaneously.

Local part is just a simple *nix utilty that can simulate serial port by using pseudoterminals or simple TCP-server.

_NOTE: for now this project is highly experimental and may be removed in future, do not rely any of your work on it_

## TODO

- more verbose description
- build instructions and hw schematics
- usage examples
- improve overall code quality if solution is stable enough
- lower data-transfer delays, use UDP as alternative