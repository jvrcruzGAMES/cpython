cmake_minimum_required(VERSION 3.13)

project(python_switch_features_cmake
    VERSION 1.0.0
    LANGUAGES C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SwitchPythonEmbed)

add_executable(python_switch_features_cmake
    source/main.c
)
target_compile_options(python_switch_features_cmake PRIVATE
    -Wall
    -O2
    -ffunction-sections
)

switch_python_embed_target(python_switch_features_cmake)
switch_python_create_nro(python_switch_features_cmake
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/python_switch_features_cmake.nro"
    ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs"
)
