# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PCSpeaker C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(PCSpeaker PCSpeaker.cpp)

pico_set_program_name(PCSpeaker "PCSpeaker")
pico_set_program_version(PCSpeaker "0.1")

pico_enable_stdio_uart(PCSpeaker 0)
pico_enable_stdio_usb(PCSpeaker 1)

# Generate the WS2812 PIO header
pico_generate_pio_header(PCSpeaker ${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812/ws2812.pio)

# LVGL
set(LV_CONF_PATH ${CMAKE_CURRENT_LIST_DIR}/lv_conf.h CACHE STRING "" FORCE)
add_subdirectory(lvgl)

target_include_directories(PCSpeaker PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/drivers/gc9a01
        ${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812
        ${CMAKE_CURRENT_LIST_DIR}/lvgl
        ${CMAKE_CURRENT_LIST_DIR}/drivers/encoder
        ${CMAKE_CURRENT_LIST_DIR}/drivers/relay
        ${CMAKE_CURRENT_LIST_DIR}/drivers/tda7439
)

target_sources(PCSpeaker PRIVATE
        drivers/gc9a01/gc9a01.c
        drivers/gc9a01/gc9a01_lvgl.c
        drivers/encoder/encoder.c
        drivers/relay/relay.c
        drivers/tda7439/tda7439.c
)

target_link_libraries(PCSpeaker
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_pio
        lvgl
)

pico_add_extra_outputs(PCSpeaker)
