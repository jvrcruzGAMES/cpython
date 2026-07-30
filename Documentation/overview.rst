Overview
========

This fork keeps CPython close to upstream 3.14 while making it usable as an
embedded runtime in Nintendo Switch homebrew applications.  The port targets
devkitPro, devkitA64, libnx, and static ``.nro`` applications.

The important differences from a desktop CPython build are:

* Python is embedded and statically linked into the host application.
* The standard library is staged into the application's RomFS.
* Native Python extension modules are linked into the NRO or another
  executable title layout, not loaded from RomFS.
* Switch filesystem roots such as ``romfs:`` and ``sdmc:`` are treated as
  absolute Python paths.
* Console output, software keyboard input, controller polling, TLS helpers,
  curl-backed HTTP, and Switch environment probing are exposed through
  fork-specific modules.

The fork-specific Python modules are:

* ``switch_support`` for environment information and runtime integration flags.
* ``switch_ssl`` for mbedTLS-backed TLS configuration helpers.
* ``switch_curl`` for curl-backed HTTP and requests adapter groundwork.
* ``switch_input`` for libnx controller polling.

The public reference pages live in :doc:`reference/index`.
