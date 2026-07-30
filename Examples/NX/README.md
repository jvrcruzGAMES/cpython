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

When the host app enables `switch_input`, the demo script owns the console as
a small Python CLI. Use D-pad or the left stick to choose a feature, `A` to run
it, `X` to run all non-interactive feature checks, `L` / `R` to scroll output,
`B` to return to the menu, and `+` to exit. The menu includes direct
`switch_curl` and requests-adapter HTTP tests, a live controller event viewer,
and a small `switch_input` Flappy-style game. If `switch_input` is not
enabled, the script falls back to printing the non-interactive feature checks
once.

Build and install or stage the Switch portlibs first:

```sh
DEVKITPRO=/opt/devkitpro \
DEVKITA64=/opt/devkitpro/devkitA64 \
PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH \
Platforms/NX/build-portlibs.sh
```

Controller polling is enabled by the native example code with
`PySwitch_EnableInputIntegration()` after Python initializes. Host apps that do
not call this function can still import `switch_input`, but polling and
software keyboard helpers report unavailable.

For a staged install, pass the staged Switch portlibs prefix as
`PYTHON_PREFIX` when building either example.

The shared `requirements.txt` pins the pure-Python packages bundled into the
examples.  The current bundle installs `requests` and its dependencies with
pip into `romfs:/python_site`.

The examples initialize the libnx console before Python. Unless the C
application installs custom Python streams, this fork routes `print()`,
tracebacks, and other `sys.stdout` / `sys.stderr` writes to that console.
Python `input()` opens the libnx software keyboard. HTTP transfers through
`switch_curl` lazily initialize the libnx socket service from Python; the
console still needs an active network connection.

The examples also opt in to NX `sys` integration with
`PySwitch_EnableSysIntegration()` after Python initializes.  That adds
`sys.switch` for the demo script; host applications that do not call the opt-in
function keep the normal CPython `sys` module without NX lifecycle controls.

The examples also opt in to `switch_ssl` and `switch_curl` with
`PySwitch_EnableSSLIntegration()` and `PySwitch_EnableCurlIntegration()`.
Host applications that skip these calls can still import and probe the modules,
but TLS helpers, curl transfers, and the requests adapter stay disabled.

The examples opt in to `switch_input` with
`PySwitch_EnableInputIntegration()`. Host applications that skip this call can
still import the module, but controller polling and
`switch_input.prompt_keyboard()` stay disabled.

Host applications can choose a different third-party package location by
appending their own site-packages path to `PyConfig.module_search_paths` before
initialization.  The examples expose this as `PYTHON_SITE_PACKAGES` for Make
and `SWITCH_PYTHON_SITE_PACKAGES` for CMake.

Both examples also include `_nx_native_demo`, a native extension module linked
into the main NRO.  This demonstrates how Switch-built wheel native payloads
should be embedded into the executable image while pure Python files and
metadata remain in RomFS.
