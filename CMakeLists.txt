cmake_minimum_required(VERSION 3.12)
include(/home/pico/devkit/pico-sdk/external/pico_sdk_import.cmake)

project(ws64 C CXX ASM)      # project name
set(CMAKE_C_STANDARD 11)       # C standard
set(CMAKE_CXX_STANDARD 17)     # C++ standard

pico_sdk_init()

# compiler (output input input ...)
add_executable(${PROJECT_NAME}
	${CMAKE_CURRENT_LIST_DIR}/wiiEC.c
	${CMAKE_CURRENT_LIST_DIR}/wiiEC.h
	${CMAKE_CURRENT_LIST_DIR}/get_bootsel_button.h
	${CMAKE_CURRENT_LIST_DIR}/get_bootsel_button.c
)

# Local include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/
)

# linker (program library library ...)
target_link_libraries(${PROJECT_NAME}
	pico_stdlib
	hardware_pio
	hardware_i2c
	pico_multicore
)

# ELF -> BIN/Dis/Hex/UF2
pico_add_extra_outputs(${PROJECT_NAME})

# Enable STDIO
pico_enable_stdio_usb(${PROJECT_NAME} 1)   # Enable STDIO on USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)  # Disable STDIO on UART
