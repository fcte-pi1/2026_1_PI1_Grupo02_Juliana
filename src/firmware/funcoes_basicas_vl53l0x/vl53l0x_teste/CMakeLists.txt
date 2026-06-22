cmake_minimum_required(VERSION 3.13)

# Importa as configurações da Pico
include(pico_sdk_import.cmake)

project(vl53l0x_micromouse_project C CXX ASM)
set(CMAKE_C_STANDARD 11)

set(PICO_BOARD pico_w)

# Inicializa o SDK
pico_sdk_init()

# Agora o driver vl53l0x.c É compilado junto com o main.c
add_executable(vl53l0x_micromouse
    main.c
    vl53l0x.c
)

# Bibliotecas de hardware
target_link_libraries(vl53l0x_micromouse pico_stdlib hardware_i2c)

# Saída USB para o monitor serial
pico_enable_stdio_usb(vl53l0x_micromouse 1)
pico_enable_stdio_uart(vl53l0x_micromouse 0)

# Gera o .uf2
pico_add_extra_outputs(vl53l0x_micromouse)
