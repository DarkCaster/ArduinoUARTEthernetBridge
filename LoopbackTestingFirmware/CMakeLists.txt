cmake_minimum_required(VERSION 3.0)

set(CMAKE_TOOLCHAIN_FILE ArduinoCmakeBuilder/arduino-avr-toolchain.cmake)

if((${CMAKE_GENERATOR} STREQUAL "NMake Makefiles JOM") AND (NOT EXISTS "${CMAKE_MAKE_PROGRAM}"))
	  set(CMAKE_MAKE_PROGRAM "${CMAKE_SOURCE_DIR}/Jom_Win32/jom.exe" CACHE INTERNAL "CMAKE_MAKE_PROGRAM")
endif()

project(SerialLoopbackTester C CXX ASM)

#definitions for atmega 2560
#set(ARDUINO_AVRDUDE_BAUD "115200" CACHE STRING "avrdude baud-rate (for optiboot)")
#set(ARDUINO_AVRDUDE_MCU "atmega2560" CACHE STRING "avrdude mcu")
#set(ARDUINO_MCU "atmega2560" CACHE STRING "-mmcu define for avr-gcc compiler")
#set(ARDUINO_F_CPU "16000000L" CACHE STRING "-DF_CPU define")
#set(ARDUINO_MODEL "ARDUINO_AVR_MEGA2560" CACHE STRING "-DARDUINO_* define")
#set(ARDUINO_VARIANT "mega" CACHE STRING "arduino variant")

#definitions for atmega 328p
set(ARDUINO_AVRDUDE_BAUD "115200" CACHE STRING "avrdude baud-rate (for optiboot)")
set(ARDUINO_AVRDUDE_MCU "atmega328p" CACHE STRING "avrdude mcu")
set(ARDUINO_MCU "atmega328p" CACHE STRING "-mmcu define for avr-gcc compiler")
set(ARDUINO_F_CPU "16000000L" CACHE STRING "-DF_CPU define")
set(ARDUINO_MODEL "ARDUINO_AVR_PRO" CACHE STRING "-DARDUINO_* define")
set(ARDUINO_VARIANT "eightanaloginputs" CACHE STRING "arduino variant")

include(ArduinoCmakeBuilder/arduino-avr-config.cmake)
include(ArduinoCmakeBuilder/arduino-build-helper.cmake)

set_avr_compiler_flags()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# add main sketdh definitions
add_arduino_sketch(SerialLoopbackTester ${CMAKE_SOURCE_DIR})
add_arduino_post_target(SerialLoopbackTester)
add_arduino_upload_target(SerialLoopbackTester)
