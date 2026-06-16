cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Hardware variant selection
set(HW_VARIANT "915_TCXO" CACHE STRING "Hardware variant: 868_XTAL, 868_TCXO, 915_XTAL, or 915_TCXO")
set_property(CACHE HW_VARIANT PROPERTY STRINGS "868_XTAL" "868_TCXO" "915_XTAL" "915_TCXO")

message(STATUS "Hardware variant: ${HW_VARIANT}")

# Map HW_VARIANT to preprocessor define
if(HW_VARIANT STREQUAL "868_XTAL")
    set(HW_DEFINE "HW_RF_868_XTAL")
elseif(HW_VARIANT STREQUAL "868_TCXO")
    set(HW_DEFINE "HW_RF_868_TCXO")
elseif(HW_VARIANT STREQUAL "915_XTAL")
    set(HW_DEFINE "HW_RF_915_XTAL")
elseif(HW_VARIANT STREQUAL "915_TCXO")
    set(HW_DEFINE "HW_RF_915_TCXO")
else()
    message(FATAL_ERROR "Invalid HW_VARIANT: ${HW_VARIANT}. Must be one of: 868_XTAL, 868_TCXO, 915_XTAL, 915_TCXO")
endif()

message(STATUS "Using hardware define: ${HW_DEFINE}")

# Board revision selection
set(HW_BOARD_REV "1_0" CACHE STRING "Board revision: 1_0, 2_0")
set_property(CACHE HW_BOARD_REV PROPERTY STRINGS "1_0" "2_0")

message(STATUS "Board revision: ${HW_BOARD_REV}")

# Map board revision to preprocessor define
if(HW_BOARD_REV STREQUAL "1_0")
    set(HW_BOARD_DEFINE "HW_BOARD_REV_1_0")
elseif(HW_BOARD_REV STREQUAL "2_0")
    set(HW_BOARD_DEFINE "HW_BOARD_REV_2_0")
else()
    message(FATAL_ERROR "Invalid HW_BOARD_REV: ${HW_BOARD_REV}. Must be one of: 1_0, 2_0")
endif()

message(STATUS "Using board define: ${HW_BOARD_DEFINE}")

# Set the project name
set(CMAKE_PROJECT_NAME CmakeTest)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_SOURCE_DIR}/Core/Src/Log.c
    ${CMAKE_SOURCE_DIR}/Core/Src/Constrain.c
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/RFTask/RF_Task.c
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/MainTask/Main_task.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/Src/radio_user.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/lr_fhss_mac.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/ral_sx126x_bsp.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/ral_sx126x.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/ralf_sx126x.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/sx126x_hal.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/sx126x_lr_fhss.c
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Src/sx126x.c
    ${CMAKE_SOURCE_DIR}/Modules/ATInterface/SerialPort/portSTM32L071xx.c
    ${CMAKE_SOURCE_DIR}/Modules/ATInterface/AT_cmd.c
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/MainTask/general_sys_cmd.c
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/MainTask/auxPin_logic.c
    ${CMAKE_SOURCE_DIR}/Modules/NVMA/NVMA.c
    
    
    
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_SOURCE_DIR}/Inc
    ${CMAKE_SOURCE_DIR}/Modules/RF/Inc
    ${CMAKE_SOURCE_DIR}/Modules/RF/SX1262/Inc
    ${CMAKE_SOURCE_DIR}/Modules/ATInterface/SerialPort
    ${CMAKE_SOURCE_DIR}/Modules/ATInterface/
    ${CMAKE_SOURCE_DIR}/Modules/NVMA
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/MainTask
    ${CMAKE_SOURCE_DIR}/Modules/Tasks/RFTask
    
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    ${HW_DEFINE}
    ${HW_BOARD_DEFINE}
)

# Stop IWDG when debugging (only in Debug build)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE STOP_IWDG_IN_DEBUG)
   # target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DO_NOT_USE_IWDG=false)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE STOP_IWDG_IN_DEBUG)
   # target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DO_NOT_USE_IWDG=false)
endif()

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Output binary name: AT_LoRa_Dongle-<variant>-<board_rev>-<build_type>
# e.g. AT_LoRa_Dongle-868_XTAL-1_0-Release
string(TOLOWER "${HW_VARIANT}" HW_VARIANT_LOWER)
string(REPLACE "_" "-" HW_VARIANT_LOWER "${HW_VARIANT_LOWER}")
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
set(OUTPUT_NAME "AT_LoRa_Dongle-${HW_VARIANT_LOWER}-v${HW_BOARD_REV}-${BUILD_TYPE_LOWER}")

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}.bin
    COMMENT "Generating ${OUTPUT_NAME}.hex / .bin"
)
