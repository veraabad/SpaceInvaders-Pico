cmake_minimum_required(VERSION 3.13)

# Set board to Pico 2
set(PICO_BOARD pico2_w)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize the SDK directly
include(/Users/abadvera/repos/pico-sdk/pico_sdk_init.cmake)

project(space_invaders_pico C CXX ASM)
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include_directories("src/include")

# ── Library target ────────────────────────────────────────────────────────────
add_library(ili9163c STATIC
    src/ili9163c.cpp
)

target_include_directories(ili9163c PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ili9163c PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
)

# ── Example executable ────────────────────────────────────────────────────────
add_executable(space_invaders_pico
    src/main.cpp
    src/utility.cpp
    src/sprites.cpp
)

target_link_libraries(space_invaders_pico
    ili9163c
    pico_stdlib
)

# Enable USB serial output for printf
pico_enable_stdio_usb(space_invaders_pico 1)
pico_enable_stdio_uart(space_invaders_pico 0)

pico_add_extra_outputs(space_invaders_pico)
