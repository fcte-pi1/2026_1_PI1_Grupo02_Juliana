cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(test_hardware C CXX ASM)

pico_sdk_init()

set(FW ${CMAKE_CURRENT_LIST_DIR}/..)
set(VL53_DIR ${FW}/funcoes_basicas_vl53l0x/vl53l0x_firmware)

add_executable(test_hardware
    test_all.cpp
    ${FW}/motores/Motor.cpp
    ${FW}/energia/EnergyMonitor.cpp
    ${FW}/funcoes_basicas_sensor_tensao/SENSTEN/voltage_sensor.c
    ${FW}/funcoes_sensor_corrente/current_sensor.c
)

target_include_directories(test_hardware PRIVATE
    ${FW}/motores
    ${FW}/energia
    ${FW}/funcoes_basicas_sensor_tensao/SENSTEN
    ${FW}/funcoes_sensor_corrente
)

pico_enable_stdio_usb(test_hardware 1)
pico_enable_stdio_uart(test_hardware 0)

target_link_libraries(test_hardware
    pico_stdlib
    hardware_gpio
    hardware_pwm
    hardware_adc
    hardware_i2c
    pico_cyw43_arch_none
)

pico_add_extra_outputs(test_hardware)

# Scan I2C em todos os pares de pinos (diagnostico de fiacao, nao mexe em motores).
add_executable(pin_scan pin_scan.cpp)
pico_enable_stdio_usb(pin_scan 1)
pico_enable_stdio_uart(pin_scan 0)
target_link_libraries(pin_scan pico_stdlib hardware_gpio hardware_i2c)
pico_add_extra_outputs(pin_scan)
