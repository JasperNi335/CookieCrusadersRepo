# Minimal Pico W MFCC feature extractor project

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# VS Code Pico extension helper (safe to include if present)
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

# Select board
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Pico SDK
include(pico_sdk_import.cmake)

project(kws_features C CXX ASM)

# Init SDK
pico_sdk_init()

# Sources
add_executable(kws_features
    src/main.c
    src/audio.c
    src/mfcc.c
)

# Link libs
target_link_libraries(kws_features
    pico_stdlib
    hardware_adc
)

# Use USB for printf; disable UART
pico_enable_stdio_usb(kws_features 1)
pico_enable_stdio_uart(kws_features 0)

# Create UF2, map/hex etc.
pico_add_extra_outputs(kws_features)