cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(micromouse_gyro C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(micromouse_gyro
    main.cpp
    mpu6050_gyro.cpp
)

target_link_libraries(micromouse_gyro
    pico_stdlib
    hardware_i2c
)

pico_enable_stdio_usb(micromouse_gyro 1)
pico_enable_stdio_uart(micromouse_gyro 0)
pico_add_extra_outputs(micromouse_gyro)