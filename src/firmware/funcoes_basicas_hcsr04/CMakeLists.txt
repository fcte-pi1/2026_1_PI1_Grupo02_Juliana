cmake_minimum_required(VERSION 3.17)

# Pico SDK bootstrap — must come before project()
set(PICO_SDK_PATH "$ENV{USERPROFILE}/.pico-sdk/sdk/2.2.0" CACHE PATH "Pico SDK path")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(hcsr04_micromouse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Target board: Raspberry Pi Pico W
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(hcsr04_micromouse
    main.cpp
    hcsr04.cpp
)

target_link_libraries(hcsr04_micromouse
    pico_stdlib
    hardware_gpio
)

# Enable USB serial (printf → USB CDC) and disable UART serial
pico_enable_stdio_usb(hcsr04_micromouse 1)
pico_enable_stdio_uart(hcsr04_micromouse 0)

# Generate .uf2, .hex, .bin and .map output files
pico_add_extra_outputs(hcsr04_micromouse)
