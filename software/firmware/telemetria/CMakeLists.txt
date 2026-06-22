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

project(telemetria C CXX ASM)

pico_sdk_init()

add_executable(telemetria
    main.cpp
    MazeMapper.cpp
)

target_include_directories(telemetria PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(telemetria 1)
pico_enable_stdio_uart(telemetria 0)

target_link_libraries(telemetria
    pico_stdlib
)

pico_add_extra_outputs(telemetria)
