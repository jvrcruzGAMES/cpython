# CPython Switch CMake Example

This example embeds this CPython fork in a Nintendo Switch homebrew project
using devkitPro's CMake toolchain.

The native example enables controller input at runtime with
`PySwitch_EnableInputIntegration()`. Host apps that skip this call can still
import `switch_input`, but controller polling and software keyboard helpers
report unavailable.

The reusable helper module lives in:

```text
cmake/SwitchPythonEmbed.cmake
```

It provides:

* `switch_python_embed_target(<target>)`
* `switch_python_add_static_module(<target> NAME <module> SOURCES <files...>)`
* `switch_python_prepare_romfs(<target> <romfs_dir> <out_var>)`
* `switch_python_install_python_packages(<target> <prepared_romfs_dir> ...)`
* `switch_python_create_nro(<target> OUTPUT <file> ROMFS <app_romfs_dir>)`
* `switch_python_create_nro(<target> OUTPUT <file> PREPARED_ROMFS <dir>)`

The module exposes these cache variables:

* `SWITCH_PYTHON_PREFIX`, defaulting to `$DEVKITPRO/portlibs/switch`
* `SWITCH_PYTHON_PC`, defaulting to `python-3.14-embed`
* `SWITCH_PYTHON_HOME`, defaulting to `romfs:/python`
* `SWITCH_PYTHON_STDLIB`, defaulting to `romfs:/python/lib/python3.14`
* `SWITCH_PYTHON_EXAMPLES`, defaulting to `romfs:/python_examples`
* `SWITCH_PYTHON_SITE_PACKAGES`, defaulting to `romfs:/python_site`
* `SWITCH_PYTHON_PACKAGE_TARGET`, defaulting to `python_site`
* `SWITCH_PYTHON_PIP`, defaulting to `python3;-m;pip`
* `SWITCH_PYTHON_PKG_CONFIG_EXECUTABLE`, defaulting to `/usr/bin/pkg-config`
* `SWITCH_PYTHON_PKG_CONFIG_LIBDIR`, defaulting to
  `$DEVKITPRO/portlibs/switch/lib/pkgconfig`

For compatibility with the original example command, `PYTHON_PREFIX` is also
accepted and copied into `SWITCH_PYTHON_PREFIX`.

Install devkitPro's CMake package if the wrapper reports
`env: 'cmake': No such file or directory`:

```sh
sudo dkp-pacman -S cmake
```

## Build

From the repository root:

```sh
DEVKITPRO=/opt/devkitpro \
DEVKITA64=/opt/devkitpro/devkitA64 \
PATH=/opt/devkitpro/portlibs/switch/bin:/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH \
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  -S Examples/NX/cmake \
  -B Examples/NX/cmake/build

/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  --build Examples/NX/cmake/build
```

For a staged CPython install:

```sh
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  -S Examples/NX/cmake \
  -B Examples/NX/cmake/build \
  -DPYTHON_PREFIX=/tmp/python-switch-stage/opt/devkitpro/portlibs/switch

/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  --build Examples/NX/cmake/build
```

The output is:

```text
Examples/NX/cmake/build/python_switch_features_cmake.nro
```

The example calls `switch_python_prepare_romfs` to copy the application's RomFS
contents into a temporary directory inside the CMake build tree, then overlays
the installed Python runtime at `python/`.  The prepared directory is passed to
`switch_python_create_nro` with `PREPARED_ROMFS`, which forwards it to
`nx_create_nro`.

Package installation is explicit: the example asks
`switch_python_install_python_packages` to create
`python_switch_features_cmake_install_python_packages` from `../requirements.txt`.
The helper can install package names, a requirements file, or a pyproject
directory into the prepared RomFS.  The default package bundle includes
`requests` and its dependencies.  Override `SWITCH_PYTHON_SITE_PACKAGES` to
choose the import path appended by the host app, and override
`SWITCH_PYTHON_PACKAGE_TARGET` to choose the matching RomFS staging directory.

```sh
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  -S . -B build \
  -DSWITCH_PYTHON_SITE_PACKAGES=romfs:/vendor \
  -DSWITCH_PYTHON_PACKAGE_TARGET=vendor
```

If your host has no `python3 -m pip`, set `SWITCH_PYTHON_PIP` to the pip
command list to use.

```sh
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  -S . -B build \
  "-DSWITCH_PYTHON_PIP=python3.14;-m;pip"
```

Use package names directly:

```cmake
switch_python_install_python_packages(my_packages "${prepared_romfs}"
  PACKAGES requests certifi
  DEPENDS my_prepare_romfs
)
```

Use a requirements file:

```cmake
switch_python_install_python_packages(my_packages "${prepared_romfs}"
  REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
  DEPENDS my_prepare_romfs
)
```

Use a local pyproject:

```cmake
switch_python_install_python_packages(my_packages "${prepared_romfs}"
  PYPROJECT "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml"
  DEPENDS my_prepare_romfs
)
```

When `switch_input` is built, `romfs:/python_examples/demo.py` controls the
libnx console as a Python CLI: D-pad or left stick selects a feature, `A` runs
it, `X` runs every non-interactive feature, `L` / `R` scroll output, `B`
returns to the menu, and `+` exits. The menu includes direct `switch_curl` and
requests-adapter HTTP tests, a live controller event viewer, and a small
`switch_input` Flappy-style game. Without the native input backend, the demo
falls back to printing all non-interactive examples once.

RomFS is only used for readable Python files and data. Native wheels, native
extension modules, `.so`, `.nro`, `.nso`, and `.elf` payloads are rejected by
`switch_python_prepare_romfs`; link native modules into the NRO or package
executable code in an executable title layout.

## Native Wheel Modules

The example includes `_nx_native_demo`, a tiny C extension module that
represents the native payload from a Switch-built wheel.  Its code is linked
into the main NRO, so it lands in the executable image instead of RomFS.

Add the source to the target:

```cmake
switch_python_add_static_module(python_switch_features_cmake
  NAME _nx_native_demo
  SOURCES source/nx_native_demo.c
)
```

Register the module before `Py_InitializeFromConfig()`:

```c
PyMODINIT_FUNC PyInit__nx_native_demo(void);

PyImport_AppendInittab("_nx_native_demo", PyInit__nx_native_demo);
```

Python can then import it normally:

```python
import _nx_native_demo
```

For a real wheel, build the extension sources with the devkitA64 compiler and
the CPython embed cflags from `python-3.14-embed.pc`, link the resulting object
or static archive into the NRO, and install only pure Python files, package
metadata, and data files into `romfs:/python_site`.

The example initializes the libnx console before Python starts. With the
default Switch streams, Python `print()` and tracebacks appear on that console,
and Python `input()` opens the libnx software keyboard. HTTP transfers through
`switch_curl` lazily initialize the libnx socket service from Python; the
console still needs an active network connection.

After Python initializes, the example calls `PySwitch_EnableSysIntegration()`
from `switch_support.h`.  This opt-in adds `sys.switch` for NX lifecycle and
console helpers; it is disabled by default for host applications that do not
request it.

The example also calls `PySwitch_EnableSSLIntegration()` and
`PySwitch_EnableCurlIntegration()` before running the script. Without those
opt-ins, `switch_ssl` and `switch_curl` remain importable for probing, but
active TLS helpers, curl transfers, and the requests adapter are disabled.

The demo exercises filesystem roots, compression modules, HACL-backed
`hashlib`, `switch_support.backends()`, `switch_support.environment()`,
opt-in `sys.switch` helpers, `switch_ssl.RAND_bytes()`,
`switch_ssl.create_default_context()`,
`switch_curl.version_info()`, curl-style handles, direct `switch_curl` HTTP,
requests HTTP through the adapter, bundled `requests`, the requests adapter
hook, optional `switch_input` controller polling, a live controller event
viewer, a small `switch_input` Flappy-style game, and a native extension linked
into the NRO.
