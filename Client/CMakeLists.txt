project(UARTClient CXX)
cmake_minimum_required(VERSION 3.1)

message(STATUS "Building for ${CMAKE_SYSTEM_NAME}. Processor architecture is ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C compiler is ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler is ${CMAKE_CXX_COMPILER}")

#set some custom options and default values
set(ARCHSUFFIX ".${CMAKE_SYSTEM_PROCESSOR}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

#print status
message(STATUS "Current build configuration:")
message(STATUS "CMAKE_GENERATOR=${CMAKE_GENERATOR}")
message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")

include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set warnings for gcc
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Applying extra flags for GCC compiler")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wshadow \
		-Wstrict-overflow=5 -Wwrite-strings -Woverlength-strings -Winit-self -Wmissing-include-dirs \
		-Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wold-style-cast\
		-Wpacked -Wredundant-decls -Wno-inline -Wdisabled-optimization -Wfloat-equal -Wswitch-default")
endif()

#check include files
include(CheckIncludeFile)
include(CheckSymbolExists)

#check for pthread support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#qt creator trick
if(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
  include_directories("${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES}")
endif(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)

file(GLOB SOURCE_FILES ${PROJECT_SOURCE_DIR}/Src/*.cpp)

add_executable(uartclient ${SOURCE_FILES})
target_link_libraries(uartclient PRIVATE util Threads::Threads)
install(TARGETS uartclient DESTINATION bin)
