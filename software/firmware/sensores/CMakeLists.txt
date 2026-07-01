cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(PICO_BOARD pico_w CACHE STRING "Board type")

project(sensores C CXX ASM)

pico_sdk_init()

# Driver do VL53L0X vive no modulo de funcao basica; referenciamos direto.
set(VL53_DIR ${CMAKE_CURRENT_LIST_DIR}/../funcoes_basicas_vl53l0x/vl53l0x_firmware)

# --- Demo / validacao dos sensores de distancia ---
add_executable(sensores_demo
    main.cpp
    DistanceSensors.cpp
    HCSR04.cpp
    ${VL53_DIR}/vl53l0x.c
)
target_include_directories(sensores_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${VL53_DIR}
)
target_link_libraries(sensores_demo
    pico_stdlib
    hardware_gpio
    hardware_i2c
)
pico_add_extra_outputs(sensores_demo)
pico_enable_stdio_usb(sensores_demo 1)
pico_enable_stdio_uart(sensores_demo 0)
