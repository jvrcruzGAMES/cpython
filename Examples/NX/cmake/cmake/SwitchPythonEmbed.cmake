include_guard(GLOBAL)

set(_SWITCH_PYTHON_EMBED_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(_switch_python_require_nx_create_nro)
    if(COMMAND nx_create_nro)
        return()
    endif()

    if(NOT DEFINED ENV{DEVKITPRO})
        message(FATAL_ERROR "DEVKITPRO is not set; cannot load libnx CMake helpers")
    endif()

    set(_switch_python_platform_file "$ENV{DEVKITPRO}/cmake/Platform/NintendoSwitch.cmake")
    if(EXISTS "${_switch_python_platform_file}")
        list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
        if(NOT DEFINED DEVKITPRO)
            set(DEVKITPRO "$ENV{DEVKITPRO}")
        endif()
        if(NOT DEFINED NX_ROOT)
            set(NX_ROOT "${DEVKITPRO}/libnx")
        endif()
        include("${_switch_python_platform_file}")
    endif()

    if(NOT NX_ELF2NRO_EXE)
        find_program(NX_ELF2NRO_EXE NAMES elf2nro HINTS "$ENV{DEVKITPRO}/tools/bin")
    endif()
    if(NOT NX_NACPTOOL_EXE)
        find_program(NX_NACPTOOL_EXE NAMES nacptool HINTS "$ENV{DEVKITPRO}/tools/bin")
    endif()
    if(NOT NX_DEFAULT_ICON)
        find_file(NX_DEFAULT_ICON NAMES default_icon.jpg HINTS "${NX_ROOT}" NO_CMAKE_FIND_ROOT_PATH)
    endif()

    if(NOT COMMAND nx_create_nro)
        message(FATAL_ERROR
            "nx_create_nro is unavailable. Configure with the devkitPro Switch "
            "toolchain, or install/update libnx CMake support under "
            "$ENV{DEVKITPRO}/cmake.")
    endif()
endfunction()

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
set(SWITCH_PYTHON_SITE_PACKAGES "romfs:/python_site" CACHE STRING
    "Site-packages path appended by the embedded runtime on Switch")
set(SWITCH_PYTHON_PACKAGE_TARGET "python_site" CACHE STRING
    "Prepared RomFS directory used for bundled pure-Python third-party packages")
set(SWITCH_PYTHON_PIP "python3;-m;pip" CACHE STRING
    "Host pip command used to bundle pure-Python third-party packages")
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
        SWITCH_PYTHON_SITE_PACKAGES="${SWITCH_PYTHON_SITE_PACKAGES}"
        SWITCH_PYTHON_STACK_SIZE=${SWITCH_PYTHON_STACK_SIZE}
    )
    target_link_options("${target}" PRIVATE -Wl,-z,notext -Wl,-u,__stacksize__)
    target_link_libraries("${target}" PRIVATE ${SWITCH_PYTHON_LIBS})
endfunction()

function(switch_python_add_static_module target)
    set(options)
    set(one_value_args NAME INIT)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(SWITCH_PYTHON_STATIC_MODULE
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT SWITCH_PYTHON_STATIC_MODULE_NAME)
        message(FATAL_ERROR "switch_python_add_static_module requires NAME")
    endif()
    if(NOT SWITCH_PYTHON_STATIC_MODULE_SOURCES)
        message(FATAL_ERROR "switch_python_add_static_module requires SOURCES")
    endif()

    target_sources("${target}" PRIVATE ${SWITCH_PYTHON_STATIC_MODULE_SOURCES})
    if(SWITCH_PYTHON_STATIC_MODULE_INIT)
        target_compile_definitions("${target}" PRIVATE
            "SWITCH_PYTHON_STATIC_MODULE_${SWITCH_PYTHON_STATIC_MODULE_NAME}=${SWITCH_PYTHON_STATIC_MODULE_INIT}"
        )
    endif()
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
        COMMAND "${CMAKE_COMMAND}" -E rm -f
                "${_switch_python_staged_romfs}/python/switch_curl.py"
        COMMAND find "${_switch_python_staged_romfs}" -path "*/__pycache__/*" -delete
        COMMAND find "${_switch_python_staged_romfs}" -type d -name "__pycache__" -empty -delete
    )

    set("${out_romfs_var}" "${_switch_python_staged_romfs}" PARENT_SCOPE)
endfunction()

function(switch_python_install_python_packages target romfs_dir)
    set(options)
    set(one_value_args DESTINATION REQUIREMENTS PYPROJECT)
    set(multi_value_args PACKAGES PIP DEPENDS)
    cmake_parse_arguments(SWITCH_PYTHON_PACKAGES
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(_switch_python_package_sources)
    if(SWITCH_PYTHON_PACKAGES_REQUIREMENTS)
        list(APPEND _switch_python_package_sources "${SWITCH_PYTHON_PACKAGES_REQUIREMENTS}")
    endif()
    if(SWITCH_PYTHON_PACKAGES_PYPROJECT)
        list(APPEND _switch_python_package_sources "${SWITCH_PYTHON_PACKAGES_PYPROJECT}")
    endif()
    if(SWITCH_PYTHON_PACKAGES_PACKAGES)
        list(APPEND _switch_python_package_sources ${SWITCH_PYTHON_PACKAGES_PACKAGES})
    endif()
    if(NOT _switch_python_package_sources)
        message(FATAL_ERROR
            "switch_python_install_python_packages requires PACKAGES, REQUIREMENTS, or PYPROJECT")
    endif()

    if(SWITCH_PYTHON_PACKAGES_DESTINATION)
        set(_switch_python_package_target "${SWITCH_PYTHON_PACKAGES_DESTINATION}")
    else()
        set(_switch_python_package_target "${SWITCH_PYTHON_PACKAGE_TARGET}")
    endif()
    if(SWITCH_PYTHON_PACKAGES_PIP)
        set(_switch_python_pip ${SWITCH_PYTHON_PACKAGES_PIP})
    else()
        set(_switch_python_pip ${SWITCH_PYTHON_PIP})
    endif()

    set(_switch_python_package_dir "${romfs_dir}/${_switch_python_package_target}")
    set(_switch_python_package_commands
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${_switch_python_package_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_switch_python_package_dir}"
    )

    if(SWITCH_PYTHON_PACKAGES_REQUIREMENTS OR SWITCH_PYTHON_PACKAGES_PACKAGES)
        set(_switch_python_pip_args
            install --disable-pip-version-check --no-compile
            --only-binary=:all: --implementation py --python-version 3.14
            --abi none --platform any
            --target "${_switch_python_package_dir}"
        )
        if(SWITCH_PYTHON_PACKAGES_REQUIREMENTS)
            list(APPEND _switch_python_pip_args -r "${SWITCH_PYTHON_PACKAGES_REQUIREMENTS}")
        endif()
        if(SWITCH_PYTHON_PACKAGES_PACKAGES)
            list(APPEND _switch_python_pip_args ${SWITCH_PYTHON_PACKAGES_PACKAGES})
        endif()
        list(APPEND _switch_python_package_commands
            COMMAND ${_switch_python_pip} ${_switch_python_pip_args}
        )
    endif()

    if(SWITCH_PYTHON_PACKAGES_PYPROJECT)
        get_filename_component(_switch_python_pyproject
            "${SWITCH_PYTHON_PACKAGES_PYPROJECT}" ABSOLUTE)
        if(IS_DIRECTORY "${_switch_python_pyproject}")
            set(_switch_python_pyproject_dir "${_switch_python_pyproject}")
        else()
            get_filename_component(_switch_python_pyproject_dir
                "${_switch_python_pyproject}" DIRECTORY)
        endif()
        list(APPEND _switch_python_package_commands
            COMMAND ${_switch_python_pip}
                    install --disable-pip-version-check --no-compile
                    --target "${_switch_python_package_dir}"
                    "${_switch_python_pyproject_dir}"
        )
    endif()

    add_custom_target("${target}"
        ${_switch_python_package_commands}
        COMMAND /bin/sh
                "${_SWITCH_PYTHON_EMBED_MODULE_DIR}/check-no-native-romfs.sh"
                "${_switch_python_package_dir}"
        DEPENDS ${SWITCH_PYTHON_PACKAGES_DEPENDS}
    )
endfunction()

function(switch_python_create_nro target)
    set(options NOICON NONACP)
    set(one_value_args OUTPUT ROMFS PREPARED_ROMFS ICON NACP NAME AUTHOR VERSION)
    set(multi_value_args DEPENDS)
    cmake_parse_arguments(SWITCH_PYTHON_NRO
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(_switch_python_nro_depends)
    if(SWITCH_PYTHON_NRO_DEPENDS)
        list(APPEND _switch_python_nro_depends ${SWITCH_PYTHON_NRO_DEPENDS})
    endif()

    if(SWITCH_PYTHON_NRO_PREPARED_ROMFS)
        set(_switch_python_staged_romfs "${SWITCH_PYTHON_NRO_PREPARED_ROMFS}")
    else()
        if(NOT SWITCH_PYTHON_NRO_ROMFS)
            set(SWITCH_PYTHON_NRO_ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs")
        endif()
        switch_python_prepare_romfs("${target}_prepare_python_romfs"
            "${SWITCH_PYTHON_NRO_ROMFS}"
            _switch_python_staged_romfs
        )
        list(APPEND _switch_python_nro_depends "${target}_prepare_python_romfs")
    endif()
    if(NOT SWITCH_PYTHON_NRO_OUTPUT)
        set(SWITCH_PYTHON_NRO_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}.nro")
    endif()

    if(_switch_python_nro_depends)
        add_dependencies("${target}" ${_switch_python_nro_depends})
    endif()
    _switch_python_require_nx_create_nro()

    set(_switch_python_nro_args
        OUTPUT "${SWITCH_PYTHON_NRO_OUTPUT}"
        ROMFS "${_switch_python_staged_romfs}"
    )
    if(SWITCH_PYTHON_NRO_ICON)
        list(APPEND _switch_python_nro_args ICON "${SWITCH_PYTHON_NRO_ICON}")
    endif()
    if(SWITCH_PYTHON_NRO_NOICON)
        list(APPEND _switch_python_nro_args NOICON)
    endif()
    if(SWITCH_PYTHON_NRO_NACP)
        list(APPEND _switch_python_nro_args NACP "${SWITCH_PYTHON_NRO_NACP}")
    endif()
    if(SWITCH_PYTHON_NRO_NONACP)
        list(APPEND _switch_python_nro_args NONACP)
    endif()

    if(SWITCH_PYTHON_NRO_NAME OR SWITCH_PYTHON_NRO_AUTHOR OR SWITCH_PYTHON_NRO_VERSION)
        if(SWITCH_PYTHON_NRO_NACP)
            message(FATAL_ERROR
                "switch_python_create_nro cannot combine NAME, AUTHOR, or VERSION with NACP")
        endif()
        if(SWITCH_PYTHON_NRO_NONACP)
            message(FATAL_ERROR
                "switch_python_create_nro cannot combine NAME, AUTHOR, or VERSION with NONACP")
        endif()

        set(_switch_python_nacp_args
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}.nacp"
        )
        if(SWITCH_PYTHON_NRO_NAME)
            list(APPEND _switch_python_nacp_args NAME "${SWITCH_PYTHON_NRO_NAME}")
        endif()
        if(SWITCH_PYTHON_NRO_AUTHOR)
            list(APPEND _switch_python_nacp_args AUTHOR "${SWITCH_PYTHON_NRO_AUTHOR}")
        endif()
        if(SWITCH_PYTHON_NRO_VERSION)
            list(APPEND _switch_python_nacp_args VERSION "${SWITCH_PYTHON_NRO_VERSION}")
        endif()

        nx_generate_nacp(${_switch_python_nacp_args})
        list(APPEND _switch_python_nro_args NACP "${CMAKE_CURRENT_BINARY_DIR}/${target}.nacp")
    endif()

    nx_create_nro("${target}" ${_switch_python_nro_args})
endfunction()
