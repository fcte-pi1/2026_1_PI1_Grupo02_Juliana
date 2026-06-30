cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(PICO_BOARD pico_w CACHE STRING "Board type")

project(MOTORN20 C CXX ASM)

pico_sdk_init()

# --- Firmware principal ---
add_executable(MOTORN20
    main.c
    motor_n20.c
    encoder.c
)
target_link_libraries(MOTORN20
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_clocks
)
pico_add_extra_outputs(MOTORN20)
pico_enable_stdio_usb(MOTORN20 1)
pico_enable_stdio_uart(MOTORN20 0)

# --- Firmware de teste/validacao ---
add_executable(MOTORN20_TEST
    test_motor_n20.c
    motor_n20.c
    encoder.c
)
target_link_libraries(MOTORN20_TEST
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_clocks
)
pico_add_extra_outputs(MOTORN20_TEST)
pico_enable_stdio_usb(MOTORN20_TEST 1)
pico_enable_stdio_uart(MOTORN20_TEST 0)
