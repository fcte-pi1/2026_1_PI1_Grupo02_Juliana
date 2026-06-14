cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(micromouse C CXX ASM)

pico_sdk_init()

# Firmware de integração: usa todos os módulos
add_executable(micromouse
    main.cpp
    motores/Motor.cpp
    motores/PID.cpp
    motores/WatchdogMotor.cpp
    telemetria/MazeMapper.cpp
)

target_include_directories(micromouse PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/motores
    ${CMAKE_CURRENT_LIST_DIR}/telemetria
)

pico_enable_stdio_usb(micromouse 1)
pico_enable_stdio_uart(micromouse 0)

target_link_libraries(micromouse
    pico_stdlib
    hardware_gpio
    hardware_pwm
    pico_multicore
    pico_sync
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_add_extra_outputs(micromouse)
