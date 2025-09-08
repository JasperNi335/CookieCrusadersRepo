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
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Make sure we really build for Pico W even if cache existed before
set(PICO_BOARD pico_w CACHE STRING "Board type" FORCE)

include(pico_sdk_import.cmake)
project(kws_features C CXX ASM)
pico_sdk_init()

add_executable(kws_features
    src/main.c
    src/audio.c
    src/mfcc.c
)

# Link the right bits for Pico W LED control
target_link_libraries(kws_features
    pico_stdlib
    hardware_adc
    pico_cyw43_arch_none
)

# USB prints ON, UART prints OFF
pico_enable_stdio_usb(kws_features 1)
pico_enable_stdio_uart(kws_features 0)

pico_add_extra_outputs(kws_features)