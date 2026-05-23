cmake_minimum_required(VERSION 3.17)

# Pico SDK bootstrap — must come before project()
set(PICO_SDK_PATH "$ENV{USERPROFILE}/.pico-sdk/sdk/2.2.0" CACHE PATH "Pico SDK path")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(mpu6050_micromouse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Target board: Raspberry Pi Pico W
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(mpu6050_micromouse
    main.cpp
    mpu6050.cpp
)

target_link_libraries(mpu6050_micromouse
    pico_stdlib
    hardware_i2c
    hardware_gpio
    m
)

# Enable USB serial (printf → USB CDC) and disable UART serial
pico_enable_stdio_usb(mpu6050_micromouse 1)
pico_enable_stdio_uart(mpu6050_micromouse 0)

# Generate .uf2, .hex, .bin and .map output files
pico_add_extra_outputs(mpu6050_micromouse)
