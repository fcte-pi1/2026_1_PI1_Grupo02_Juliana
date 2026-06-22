# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Importa o SDK da Pico
include(pico_sdk_import.cmake)

project(teste_vl53l0x C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializa o SDK
pico_sdk_init()

# Define o seu arquivo de código principal
add_executable(funcaobasica
    main.c
)

# Vincula as bibliotecas do SDK
target_link_libraries(funcaobasica 
    pico_stdlib 
    hardware_i2c
)

# Ativa a saída do printf via USB
pico_enable_stdio_usb(funcaobasica 1)
pico_enable_stdio_uart(funcaobasica 0)

# Gera os arquivos finais (.elf, .uf2, etc.)
pico_add_extra_outputs(funcaobasica)