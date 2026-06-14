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

project(motores C CXX ASM)

pico_sdk_init()

add_executable(motores
    main.cpp
    Motor.cpp
    PID.cpp
    WatchdogMotor.cpp
)

target_include_directories(motores PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(motores 1)
pico_enable_stdio_uart(motores 0)

target_link_libraries(motores
    pico_stdlib
    hardware_gpio
    hardware_pwm
    pico_sync
    pico_cyw43_arch_none
)

pico_add_extra_outputs(motores)
