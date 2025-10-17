# PRISM K1 Firmware
# ESP-IDF v5.x project configuration

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF project configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set project details
set(PROJECT_NAME "prism-k1")
set(PROJECT_VER "1.0.0")

# Configure components directory
set(EXTRA_COMPONENT_DIRS
    "components/core"
    # TODO: Enable as components are implemented
    # "components/network"
    # "components/storage"
    # "components/playback"
    # "components/templates"
)

# Project declaration
project(${PROJECT_NAME})

# Set compile options for optimization
idf_build_set_property(COMPILE_OPTIONS "-O2" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-ffunction-sections" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-fdata-sections" APPEND)

# Surface linker diagnostics locally to debug silent link failures
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--verbose")
