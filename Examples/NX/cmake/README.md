# CPython Switch CMake Example

This example embeds this CPython fork in a Nintendo Switch homebrew project
using devkitPro's CMake toolchain.

The reusable helper module lives in:

```text
cmake/SwitchPythonEmbed.cmake
```

It provides:

* `switch_python_embed_target(<target>)`
* `switch_python_prepare_romfs(<target> <romfs_dir> <out_var>)`
* `switch_python_create_nro(<target> OUTPUT <file> ROMFS <dir>)`

The module exposes these cache variables:

* `SWITCH_PYTHON_PREFIX`, defaulting to `$DEVKITPRO/portlibs/switch`
* `SWITCH_PYTHON_PC`, defaulting to `python-3.14-embed`
* `SWITCH_PYTHON_HOME`, defaulting to `romfs:/python`
* `SWITCH_PYTHON_STDLIB`, defaulting to `romfs:/python/lib/python3.14`
* `SWITCH_PYTHON_EXAMPLES`, defaulting to `romfs:/python_examples`
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

The build runs `prepare_romfs` automatically and copies the staged Python
runtime into `romfs/python`.  The helper stages RomFS in a temporary directory
inside the CMake build tree, copies the application's RomFS contents into it,
then overlays the installed Python runtime at `python/` before `nx_create_nro`
builds the RomFS image.

RomFS is only used for readable Python files and data. Native wheels, native
extension modules, `.so`, `.nro`, `.nso`, and `.elf` payloads are rejected by
`switch_python_prepare_romfs`; link native modules into the NRO or package
executable code in an executable title layout.

The example initializes the libnx console before Python starts.  With the
default Switch streams, Python `print()` and tracebacks appear on that console,
and Python `input()` opens the libnx software keyboard.
