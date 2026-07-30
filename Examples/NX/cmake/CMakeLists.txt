cmake_minimum_required(VERSION 3.13)

if(NOT NINTENDO_SWITCH AND DEFINED ENV{DEVKITPRO})
    list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
    include("$ENV{DEVKITPRO}/cmake/Switch.cmake")
endif()

project(python_switch_features_cmake
    VERSION 1.0.0
    LANGUAGES C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SwitchPythonEmbed)

set(EXAMPLE_PYTHON_REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/../requirements.txt"
    CACHE FILEPATH "pip requirements bundled into this example")

add_executable(python_switch_features_cmake
    source/main.c
)
target_compile_options(python_switch_features_cmake PRIVATE
    -Wall
    -O2
    -ffunction-sections
)

switch_python_embed_target(python_switch_features_cmake)
switch_python_add_static_module(python_switch_features_cmake
    NAME _nx_native_demo
    INIT PyInit__nx_native_demo
    SOURCES source/nx_native_demo.c
)

switch_python_prepare_romfs(python_switch_features_cmake_prepare_romfs
    "${CMAKE_CURRENT_SOURCE_DIR}/romfs"
    python_switch_features_cmake_romfs
)

switch_python_install_python_packages(python_switch_features_cmake_install_python_packages
    "${python_switch_features_cmake_romfs}"
    REQUIREMENTS "${EXAMPLE_PYTHON_REQUIREMENTS}"
    DEPENDS python_switch_features_cmake_prepare_romfs
)

switch_python_create_nro(python_switch_features_cmake
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/python_switch_features_cmake.nro"
    PREPARED_ROMFS "${python_switch_features_cmake_romfs}"
    DEPENDS python_switch_features_cmake_install_python_packages
    NAME "Python Switch"
    AUTHOR "CPython Switch"
    VERSION "${PROJECT_VERSION}"
)
