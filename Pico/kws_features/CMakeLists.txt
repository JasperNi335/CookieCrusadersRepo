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

# You can keep pico_w or use pico; Wi-Fi isn’t linked here.
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# --- Point to your Pico SDK ---
if(NOT DEFINED PICO_SDK_PATH)
  set(PICO_SDK_PATH "C:/Users/Johnny Vu/.pico-sdk/sdk/2.2.0")
endif()

# Pull in the Pico SDK
include(pico_sdk_import.cmake)

project(kws_features C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# --- Sources ---
add_executable(kws_features
    src/main.c
    src/audio.c
    src/mfcc.c
)

# --- Include directories ---
target_include_directories(kws_features PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# --- I/O ---
pico_enable_stdio_usb(kws_features 1)
pico_enable_stdio_uart(kws_features 0)

# --- Libraries (NO Wi-Fi needed) ---
target_link_libraries(kws_features
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_timer
    hardware_uart
    pico_util
)

# --- Generate .uf2/.bin/.elf ---
pico_add_extra_outputs(kws_features)