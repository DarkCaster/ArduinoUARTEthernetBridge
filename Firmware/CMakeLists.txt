cmake_minimum_required(VERSION 3.0)

set(ARDUINO_AVRDUDE_BAUD "57600" CACHE STRING "avrdude baud-rate (for optiboot)")
set(ARDUINO_AVRDUDE_MCU "atmega328p" CACHE STRING "avrdude mcu")
set(ARDUINO_MCU "atmega328p" CACHE STRING "-mmcu define for avr-gcc compiler")
set(ARDUINO_F_CPU "16000000L" CACHE STRING "-DF_CPU define")
set(ARDUINO_MODEL "ARDUINO_AVR_PRO" CACHE STRING "-DARDUINO_* define")
set(ARDUINO_VARIANT "eightanaloginputs" CACHE STRING "arduino variant")

set(CMAKE_TOOLCHAIN_FILE ArduinoCmakeBuilder/arduino-avr-toolchain.cmake)

if((${CMAKE_GENERATOR} STREQUAL "NMake Makefiles JOM") AND (NOT EXISTS "${CMAKE_MAKE_PROGRAM}"))
    set(CMAKE_MAKE_PROGRAM "${CMAKE_SOURCE_DIR}/Jom_Win32/jom.exe" CACHE INTERNAL "CMAKE_MAKE_PROGRAM")
endif()

project(UARTEthernetBridge C CXX ASM)

include(ArduinoCmakeBuilder/arduino-build-helper.cmake)

set_avr_compiler_flags()

# add some internal and external libraries
add_arduino_library(SPI ${ARDUINO_LIBS_PATH})
add_arduino_library(EEPROM ${ARDUINO_LIBS_PATH})
add_arduino_library(UIPEthernet ${CMAKE_SOURCE_DIR}/Libraries SPI)
target_compile_options(UIPEthernet PRIVATE "-Wno-cpp" "-Wno-sign-compare" "-Wno-unused-label")

# add main sketdh definitions
add_arduino_sketch(UARTEthernetBridge ${CMAKE_SOURCE_DIR} EEPROM UIPEthernet)
add_arduino_post_target(UARTEthernetBridge)
add_arduino_upload_target(UARTEthernetBridge)
