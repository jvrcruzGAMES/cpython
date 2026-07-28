# CPython Switch Examples

This directory contains Nintendo Switch homebrew examples that embed this
CPython fork.

## Examples

* `make/` uses the traditional devkitPro/libnx Makefile project layout.
* `cmake/` uses `/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake`
  and devkitPro's `nx_create_nro()`.

Both examples run the same Python feature demo from:

```text
romfs:/python_examples/demo.py
```

Build and install or stage the Switch portlibs first:

```sh
DEVKITPRO=/opt/devkitpro \
DEVKITA64=/opt/devkitpro/devkitA64 \
PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH \
Platforms/NX/build-portlibs.sh
```

For a staged install, pass the staged Switch portlibs prefix as
`PYTHON_PREFIX` when building either example.

The examples initialize the libnx console before Python.  Unless the C
application installs custom Python streams, this fork routes `print()`,
tracebacks, and other `sys.stdout` / `sys.stderr` writes to that console.
Python `input()` opens the libnx software keyboard.
