# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
  set(PICO_SDK_PATH "C:/Users/Johnny Vu/.pico-sdk/sdk/2.2.0")
endif()

include(pico_sdk_import.cmake)

project(kws_features C CXX ASM)
pico_sdk_init()

add_executable(kws_features
    src/main.c
    src/audio.c
)

target_include_directories(kws_features PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

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