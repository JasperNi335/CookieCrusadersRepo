cmake_minimum_required(VERSION 3.16)
set(PICO_BOARD pico CACHE STRING "Board type")

# Keep this file here (YES, it's important)
include(pico_sdk_import.cmake)   # must be before project()

project(kws_features C CXX ASM)
pico_sdk_init()

add_executable(kws_features
    src/main.c
    src/audio.c
)

target_include_directories(kws_features PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_enable_stdio_usb(kws_features 1)
pico_enable_stdio_uart(kws_features 0)

target_link_libraries(kws_features
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_timer
    hardware_pwm
    pico_util
)

pico_add_extra_outputs(kws_features)