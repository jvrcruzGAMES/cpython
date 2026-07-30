# Nintendo Switch homebrew

This directory contains cross-compilation support for building CPython as a
Nintendo Switch homebrew library with devkitPro's devkitA64 toolchain and
libnx.

The target triple is `aarch64-none-elf`. The port is intended for embedding
CPython into a `.nro`; it installs `libpython*.a`, headers, pkg-config files,
and a staged `romfs` standard-library tree into Switch portlibs.

## Prerequisites

Install or update the Switch toolchain with devkitPro's package manager:

```sh
sudo dkp-pacman -Syu \
  devkitA64 \
  libnx \
  switch-tools \
  switch-zlib \
  switch-liblzma \
  switch-libzstd \
  switch-mbedtls \
  switch-curl
```

Ensure `DEVKITPRO` and `DEVKITA64` are set, and that the devkitA64 tools are on
`PATH`. If `switch-curl` is available, the build helper automatically links
the native `_switch_curl` backend used by `switch_curl.Curl.perform()` and the
requests adapter. If `switch-curl` is omitted, the build helper will still
build CPython and `switch_curl` will report curl as unavailable.

`switch_ssl` and `switch_curl` are inactive by default at runtime. Embedders
that want Python code to use TLS helpers or curl transfers must call
`PySwitch_EnableSSLIntegration()` and `PySwitch_EnableCurlIntegration()` after
`Py_InitializeFromConfig()`, or expose the matching `switch_support`
Python opt-ins deliberately.

## Portlibs install

The recommended path is the portlibs helper:

```sh
Platforms/NX/build-portlibs.sh
```

By default this installs into:

```text
$DEVKITPRO/portlibs/switch
```

The install contains:

```text
include/pythonX.Y/                 C API headers
lib/libpythonX.Y.a                 static library
lib/pkgconfig/python3-embed.pc     pkg-config metadata for embedding
share/python-switch/romfs/python/  runtime tree for application romfs
```

Copy `share/python-switch/romfs/python` into your homebrew project's `romfs`
directory. The port is configured to find the standard library at
`romfs:/python/lib/pythonX.Y`.

RomFS is treated as a readable asset store only. Do not place native wheels,
extension modules, `.so` files, `.nro` files, `.nso` files, or other executable
Python payloads in the staged RomFS tree. Native Python libraries should be
linked into the application, or packaged in an executable title layout such as
ExeFS when targeting a title format that supports it.

For a staged install without writing to `$DEVKITPRO`, use:

```sh
Platforms/NX/build-portlibs.sh --destdir /tmp/python-switch-stage
```

The native controller input backend is built for Switch portlibs by default.
It is disabled at runtime until the host NRO opts in with
`PySwitch_EnableInputIntegration()`. The public `switch_input` Python module
uses that backend for libnx pad polling and software keyboard prompts.

## Embedding

A minimal embedding example is available in `Platforms/NX/templates`. One way
to start a project from it is:

```sh
mkdir -p my-python-app/source my-python-app/romfs
cp Platforms/NX/templates/Makefile my-python-app/
cp Platforms/NX/templates/embed-main.c my-python-app/source/main.c
cp -R "$DEVKITPRO/portlibs/switch/share/python-switch/romfs/python" \
  my-python-app/romfs/
(cd my-python-app && make)
```

The template links with:

```sh
PKG_CONFIG_LIBDIR=$DEVKITPRO/portlibs/switch/lib/pkgconfig \
pkg-config --cflags --libs python3-embed
```

A complete feature example is available in both `Examples/NX/make` and
`Examples/NX/cmake`. Each embeds CPython in a libnx application and runs
examples for the Switch filesystem roots, compression modules, `hashlib`,
`switch_support`, `switch_ssl`, `switch_curl` groundwork, and optional
host-enabled `switch_input` controller polling:

```sh
cd Examples/NX/make
make prepare-romfs
make
```

The CMake example should be configured with the portlibs wrapper:

```sh
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  -S Examples/NX/cmake \
  -B Examples/NX/cmake/build
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
  --build Examples/NX/cmake/build
```

## Manual configure

Build a native CPython for the build machine first, then configure the Switch
build out of tree:

```sh
mkdir -p build/build-python build/nx
(cd build/build-python && ../../configure && make -j)

cd build/nx
CONFIG_SITE=../../Platforms/NX/config.site-aarch64-none-elf \
CC=aarch64-none-elf-gcc \
CXX=aarch64-none-elf-g++ \
AR=aarch64-none-elf-gcc-ar \
RANLIB=aarch64-none-elf-gcc-ranlib \
READELF=aarch64-none-elf-readelf \
PKG_CONFIG=aarch64-none-elf-pkg-config \
PKG_CONFIG_LIBDIR="$DEVKITPRO/portlibs/switch/lib/pkgconfig" \
CPPFLAGS="-D__SWITCH__ -I$DEVKITPRO/libnx/include -I$DEVKITPRO/portlibs/switch/include" \
CFLAGS="-fPIE" \
LDFLAGS="-specs=$DEVKITPRO/libnx/switch.specs -L$DEVKITPRO/libnx/lib -L$DEVKITPRO/portlibs/switch/lib" \
LIBS="-lnx -lm -lz -llzma -lzstd -lmbedtls -lmbedx509 -lmbedcrypto" \
../../configure \
  --host=aarch64-none-elf \
  --build=$(../../config.guess) \
  --with-build-python=$(pwd)/../build-python/python \
  --prefix="$DEVKITPRO/portlibs/switch" \
  --exec-prefix="$DEVKITPRO/portlibs/switch" \
  host_prefix="romfs:/python" \
  host_exec_prefix="romfs:/python" \
  --disable-ipv6 \
  --disable-test-modules \
  --without-mimalloc \
  --without-ensurepip
```

Then run:

```sh
make -j
```

## Current limitations

The port disables shared extension modules, process control, POSIX user/group
database modules, terminal UI modules, `mmap`, OpenSSL, sqlite, ctypes,
multiprocessing, subprocess support, and ensurepip by default.

`zlib`, `_lzma`, and `_zstd` are built from `switch-zlib`, `switch-liblzma`,
and `switch-libzstd`. `switch_support` reports the linked compression,
mbedTLS, optional switch-curl backends, firmware, and product model. CPython's
OpenSSL-specific `_ssl` and
`_hashlib` modules remain disabled; `hashlib` uses CPython's built-in digest
modules for the supported embedded algorithms.
