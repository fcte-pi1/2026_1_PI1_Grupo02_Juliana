cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(PICO_BOARD pico_w CACHE STRING "Board type")

project(SENSCORRENTE C CXX ASM)

pico_sdk_init()

# --- Firmware principal ---
add_executable(SENSCORRENTE
    main.c
    current_sensor.c
)
target_link_libraries(SENSCORRENTE
    pico_stdlib
    hardware_adc
)
pico_add_extra_outputs(SENSCORRENTE)
pico_enable_stdio_usb(SENSCORRENTE 1)
pico_enable_stdio_uart(SENSCORRENTE 0)

# --- Firmware de teste/validacao ---
add_executable(SENSCORRENTE_TEST
    test_current_sensor.c
    current_sensor.c
)
target_link_libraries(SENSCORRENTE_TEST
    pico_stdlib
    hardware_adc
)
pico_add_extra_outputs(SENSCORRENTE_TEST)
pico_enable_stdio_usb(SENSCORRENTE_TEST 1)
pico_enable_stdio_uart(SENSCORRENTE_TEST 0)
