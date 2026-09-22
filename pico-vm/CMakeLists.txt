cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc.)

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

if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(VM C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Find all C and header files in this directory
file(GLOB VM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

# Add executable
add_executable(VM ${VM_SOURCES})

pico_set_program_name(VM "VM")
pico_set_program_version(VM "0.1")

# Enable/disable output over UART/USB
pico_enable_stdio_uart(VM 0)
pico_enable_stdio_usb(VM 1)

# Add the standard library
target_link_libraries(VM
    pico_stdlib
)

# Add standard include files
target_include_directories(VM PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Generate UF2, BIN, HEX, etc.
pico_add_extra_outputs(VM)