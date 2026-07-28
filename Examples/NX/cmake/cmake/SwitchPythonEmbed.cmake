include_guard(GLOBAL)

set(_SWITCH_PYTHON_EMBED_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(SWITCH_PYTHON_PREFIX "$ENV{DEVKITPRO}/portlibs/switch" CACHE PATH
    "Switch portlibs prefix containing this CPython install")
if(DEFINED PYTHON_PREFIX AND NOT "${PYTHON_PREFIX}" STREQUAL "${SWITCH_PYTHON_PREFIX}")
    set(SWITCH_PYTHON_PREFIX "${PYTHON_PREFIX}" CACHE PATH
        "Switch portlibs prefix containing this CPython install" FORCE)
endif()
set(SWITCH_PYTHON_PC "python-3.14-embed" CACHE STRING
    "pkg-config module for embedded CPython")
set(SWITCH_PYTHON_HOME "romfs:/python" CACHE STRING
    "Python home path used by the embedded runtime on Switch")
set(SWITCH_PYTHON_STDLIB "romfs:/python/lib/python3.14" CACHE STRING
    "Python stdlib path used by the embedded runtime on Switch")
set(SWITCH_PYTHON_EXAMPLES "romfs:/python_examples" CACHE STRING
    "Example Python script search path used by the embedded runtime on Switch")
set(SWITCH_PYTHON_STACK_SIZE "33554432" CACHE STRING
    "Main-thread stack size for embedded CPython on Switch")
set(SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config" CACHE FILEPATH
    "Host pkg-config executable used to query the CPython embed pc file")
set(SWITCH_PYTHON_PKG_CONFIG_LIBDIR "${SWITCH_PYTHON_PREFIX}/lib/pkgconfig" CACHE PATH
    "pkg-config search directory for the CPython embed pc file")

function(switch_python_require)
    if(NOT DEFINED ENV{DEVKITPRO})
        message(FATAL_ERROR "DEVKITPRO is not set")
    endif()

    if(NOT EXISTS "${SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE}")
        find_program(_switch_python_pkg_config
            NAMES pkg-config
            PATHS /usr/bin /bin
            NO_DEFAULT_PATH
        )
        if(NOT _switch_python_pkg_config)
            message(FATAL_ERROR "Could not find host pkg-config")
        endif()
        set(SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE "${_switch_python_pkg_config}"
            CACHE FILEPATH "Host pkg-config executable used to query the CPython embed pc file" FORCE)
    endif()

    set(_switch_python_pkg_config_env
        "PKG_CONFIG_LIBDIR=${SWITCH_PYTHON_PKG_CONFIG_LIBDIR}"
        "PKG_CONFIG_PATH="
        "PKG_CONFIG_SYSROOT_DIR="
    )
    set(_switch_python_pkg_config_args
        "--define-variable=prefix=${SWITCH_PYTHON_PREFIX}"
        "--define-variable=exec_prefix=${SWITCH_PYTHON_PREFIX}"
        "${SWITCH_PYTHON_PC}"
    )

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env ${_switch_python_pkg_config_env}
                "${SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE}" --cflags ${_switch_python_pkg_config_args}
        OUTPUT_VARIABLE _switch_python_cflags
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _switch_python_cflags_result
    )
    if(NOT _switch_python_cflags_result EQUAL 0)
        message(FATAL_ERROR "Could not query ${SWITCH_PYTHON_PC} cflags with pkg-config")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env ${_switch_python_pkg_config_env}
                "${SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE}" --static --libs ${_switch_python_pkg_config_args}
        OUTPUT_VARIABLE _switch_python_libs
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _switch_python_libs_result
    )
    if(NOT _switch_python_libs_result EQUAL 0)
        message(FATAL_ERROR "Could not query ${SWITCH_PYTHON_PC} static libs with pkg-config")
    endif()

    separate_arguments(_switch_python_cflags_list UNIX_COMMAND "${_switch_python_cflags}")
    separate_arguments(_switch_python_libs_list UNIX_COMMAND "${_switch_python_libs}")

    set(SWITCH_PYTHON_CFLAGS "${_switch_python_cflags_list}" CACHE INTERNAL
        "Compile flags for embedded CPython on Switch")
    set(SWITCH_PYTHON_LIBS "${_switch_python_libs_list}" CACHE INTERNAL
        "Static link flags for embedded CPython on Switch")
    set(SWITCH_PYTHON_RUNTIME_DIR "${SWITCH_PYTHON_PREFIX}/share/python-switch/romfs/python" CACHE INTERNAL
        "Staged CPython runtime directory for Switch romfs")
endfunction()

function(switch_python_embed_target target)
    switch_python_require()
    target_compile_options("${target}" PRIVATE ${SWITCH_PYTHON_CFLAGS})
    target_compile_definitions("${target}" PRIVATE
        SWITCH_PYTHON_HOME="${SWITCH_PYTHON_HOME}"
        SWITCH_PYTHON_STDLIB="${SWITCH_PYTHON_STDLIB}"
        SWITCH_PYTHON_EXAMPLES="${SWITCH_PYTHON_EXAMPLES}"
        SWITCH_PYTHON_STACK_SIZE=${SWITCH_PYTHON_STACK_SIZE}
    )
    target_link_options("${target}" PRIVATE -Wl,-z,notext -Wl,-u,__stacksize__)
    target_link_libraries("${target}" PRIVATE ${SWITCH_PYTHON_LIBS})
endfunction()

function(switch_python_prepare_romfs target app_romfs_dir out_romfs_var)
    switch_python_require()

    set(_switch_python_staged_romfs "${CMAKE_CURRENT_BINARY_DIR}/${target}_staged")
    file(MAKE_DIRECTORY "${_switch_python_staged_romfs}")

    set(_switch_python_copy_app_romfs)
    if(EXISTS "${app_romfs_dir}")
        list(APPEND _switch_python_copy_app_romfs
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${app_romfs_dir}"
                    "${_switch_python_staged_romfs}"
        )
    endif()

    add_custom_target("${target}"
        COMMAND "${CMAKE_COMMAND}" -E echo
                "Staging RomFS at ${_switch_python_staged_romfs}"
        COMMAND /bin/sh
                "${_SWITCH_PYTHON_EMBED_MODULE_DIR}/check-no-native-romfs.sh"
                "${SWITCH_PYTHON_RUNTIME_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${_switch_python_staged_romfs}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${_switch_python_staged_romfs}"
        ${_switch_python_copy_app_romfs}
        COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${_switch_python_staged_romfs}/python"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${SWITCH_PYTHON_RUNTIME_DIR}"
                "${_switch_python_staged_romfs}/python"
    )

    set("${out_romfs_var}" "${_switch_python_staged_romfs}" PARENT_SCOPE)
endfunction()

function(switch_python_create_nro target)
    set(options)
    set(one_value_args OUTPUT ROMFS)
    set(multi_value_args)
    cmake_parse_arguments(SWITCH_PYTHON_NRO
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT SWITCH_PYTHON_NRO_ROMFS)
        set(SWITCH_PYTHON_NRO_ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs")
    endif()
    if(NOT SWITCH_PYTHON_NRO_OUTPUT)
        set(SWITCH_PYTHON_NRO_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}.nro")
    endif()

    switch_python_prepare_romfs("${target}_prepare_python_romfs"
        "${SWITCH_PYTHON_NRO_ROMFS}"
        _switch_python_staged_romfs
    )
    add_dependencies("${target}" "${target}_prepare_python_romfs")
    nx_create_nro("${target}"
        OUTPUT "${SWITCH_PYTHON_NRO_OUTPUT}"
        ROMFS "${_switch_python_staged_romfs}"
    )
endfunction()
