cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(PICO_BOARD pico_w CACHE STRING "Board type")

project(energia C CXX ASM)

pico_sdk_init()

# Driver de tensao vive no modulo SENSTEN; referenciamos direto pra nao duplicar.
set(SENSTEN_DIR ${CMAKE_CURRENT_LIST_DIR}/../funcoes_basicas_sensor_tensao/SENSTEN)

# --- Demo / validacao do monitor de energia ---
add_executable(energia_demo
    main.cpp
    EnergyMonitor.cpp
    ${SENSTEN_DIR}/voltage_sensor.c
)
target_include_directories(energia_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${SENSTEN_DIR}
)
target_link_libraries(energia_demo
    pico_stdlib
    hardware_adc
)
pico_add_extra_outputs(energia_demo)
pico_enable_stdio_usb(energia_demo 1)
pico_enable_stdio_uart(energia_demo 0)
