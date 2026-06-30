cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(PICO_BOARD pico_w CACHE STRING "Board type")

project(SENSTEN C CXX ASM)

pico_sdk_init()

# --- Firmware principal ---
add_executable(SENSTEN
    main.c
    voltage_sensor.c
)
target_link_libraries(SENSTEN
    pico_stdlib
    hardware_adc
)
pico_add_extra_outputs(SENSTEN)
pico_enable_stdio_usb(SENSTEN 1)
pico_enable_stdio_uart(SENSTEN 0)

# --- Firmware de teste/validacao ---
add_executable(SENSTEN_TEST
    test_voltage_sensor.c
    voltage_sensor.c
)
target_link_libraries(SENSTEN_TEST
    pico_stdlib
    hardware_adc
)
pico_add_extra_outputs(SENSTEN_TEST)
pico_enable_stdio_usb(SENSTEN_TEST 1)
pico_enable_stdio_uart(SENSTEN_TEST 0)