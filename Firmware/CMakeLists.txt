cmake_minimum_required(VERSION 3.0)

set(CMAKE_TOOLCHAIN_FILE ArduinoCmakeBuilder/arduino-avr-toolchain.cmake)

if((${CMAKE_GENERATOR} STREQUAL "NMake Makefiles JOM") AND (NOT EXISTS "${CMAKE_MAKE_PROGRAM}"))
	  set(CMAKE_MAKE_PROGRAM "${CMAKE_SOURCE_DIR}/Jom_Win32/jom.exe" CACHE INTERNAL "CMAKE_MAKE_PROGRAM")
endif()

project(UARTEthernetBridge C CXX ASM)

add_definitions(-DARDUINO_MEGA2560_CONFIG)
set(ARDUINO_AVRDUDE_BAUD "115200" CACHE STRING "avrdude baud-rate (for optiboot)")
set(ARDUINO_AVRDUDE_MCU "atmega2560" CACHE STRING "avrdude mcu")
set(ARDUINO_MCU "atmega2560" CACHE STRING "-mmcu define for avr-gcc compiler")
set(ARDUINO_F_CPU "16000000L" CACHE STRING "-DF_CPU define")
set(ARDUINO_MODEL "ARDUINO_AVR_MEGA2560" CACHE STRING "-DARDUINO_* define")
set(ARDUINO_VARIANT "mega" CACHE STRING "arduino variant")

#add_definitions(-DARDUINO_PRO_MINI_CONFIG)
#add_definitions(-DUIP_SOCKET_NUMPACKETS=2)
#set(ARDUINO_AVRDUDE_BAUD "115200" CACHE STRING "avrdude baud-rate (for optiboot)")
#set(ARDUINO_AVRDUDE_MCU "atmega328p" CACHE STRING "avrdude mcu")
#set(ARDUINO_MCU "atmega328p" CACHE STRING "-mmcu define for avr-gcc compiler")
#set(ARDUINO_F_CPU "16000000L" CACHE STRING "-DF_CPU define")
#set(ARDUINO_MODEL "ARDUINO_AVR_PRO" CACHE STRING "-DARDUINO_* define")
#set(ARDUINO_VARIANT "eightanaloginputs" CACHE STRING "arduino variant")

include(ArduinoCmakeBuilder/arduino-avr-config.cmake)
include(ArduinoCmakeBuilder/arduino-build-helper.cmake)

set_avr_compiler_flags()

#override some config definitions for UIP library to reduce RAM footprint and TCP latency
#this is cruical for arduino pro mini / atmega 328p, project will not fit into 2KiB ram without these options
add_definitions(-DUIP_CLIENT_TIMER=0)
add_definitions(-DUIP_CONF_BROADCAST=0)
add_definitions(-DUIP_CONF_UDP_CONNS=1)
add_definitions(-DUIP_CONF_MAX_CONNECTIONS=2)

# add some internal and external libraries
add_arduino_library(SPI ${ARDUINO_LIBS_PATH})
add_arduino_library(UIPEthernet ${CMAKE_SOURCE_DIR}/Libraries SPI)
target_compile_options(UIPEthernet PRIVATE "-Wno-cpp" "-Wno-sign-compare" "-Wno-unused-label")

# add main sketdh definitions
add_arduino_sketch(UARTEthernetBridge ${CMAKE_SOURCE_DIR} UIPEthernet)
target_compile_options(UARTEthernetBridge PRIVATE "-Wno-cpp")
add_arduino_post_target(UARTEthernetBridge)
add_arduino_upload_target(UARTEthernetBridge)
