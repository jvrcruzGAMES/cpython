Nintendo Switch Fork APIs
=========================

This document covers APIs and behavior that are specific to this CPython fork.
For normal Python language and standard-library documentation, use the upstream
Python 3.14 documentation.

Filesystem Roots
----------------

The Switch port treats libnx filesystem roots as absolute Python paths:

* ``romfs:``
* ``romfs:/``
* ``sdmc:``
* ``sdmc:/``

Both ``romfs:`` and ``romfs:/`` are accepted.  The same applies to ``sdmc:``.
This lets embedded applications use natural libnx paths while still using
Python path helpers.

Examples:

.. code-block:: python

   import os
   import posixpath

   posixpath.isabs("romfs:")
   # True

   posixpath.join("romfs:", "python.py")
   # 'romfs:/python.py'

   posixpath.join("romfs:/Lib", "/site.py")
   # 'romfs:/site.py'

   posixpath.splitroot("sdmc:/switch/python")
   # ('sdmc:', '/', 'switch/python')

Bytes paths are also supported:

.. code-block:: python

   posixpath.join(b"romfs:", b"python.py")
   # b'romfs:/python.py'

Runtime Prefix
--------------

The portlibs build configures CPython to load its standard library from:

.. code-block:: text

   romfs:/python/lib/python3.14

Applications should copy the staged runtime tree into their homebrew romfs:

.. code-block:: text

   $DEVKITPRO/portlibs/switch/share/python-switch/romfs/python

RomFS is for readable Python source, bytecode, and data files only.  Native
wheels, extension modules, shared libraries, NSO/NRO payloads, and other
executable Python content must not be loaded from RomFS.  For this fork, native
Python modules should be linked into the application image.  If a project uses a
title packaging format with executable content support, native payloads belong
in that executable layout, such as ExeFS, not in RomFS.

The runtime prefix can be changed when building:

.. code-block:: sh

   Platforms/NX/build-portlibs.sh --runtime-prefix sdmc:/python

Console Streams
---------------

On Switch, CPython installs console-backed standard streams by default when
the embedding application has not provided its own Python ``sys.stdin``,
``sys.stdout``, or ``sys.stderr`` objects.

``sys.stdout`` and ``sys.stderr`` write to the active libnx console, so normal
Python output works in an ``nxlink`` or framebuffer-console style homebrew
application:

.. code-block:: python

   print("Hello from Python on Switch", flush=True)

``input()`` reads through libnx's software keyboard applet.  If the user
cancels the keyboard, ``input()`` receives end-of-file behavior.  Applications
that need custom logging, sockets, files, overlays, or their own keyboard UI
can still replace ``sys.stdin``, ``sys.stdout``, and ``sys.stderr`` after
initialization, or configure their embedding layer to install custom streams.

``switch_ssl``
--------------

``switch_ssl`` is a Switch-specific built-in module.  It reports the backend
libraries compiled into the Switch Python runtime.

.. code-block:: python

   import switch_ssl

   switch_ssl.backends()
   # {
   #     'compression': {
   #         'zlib': '1.3.1',
   #         'lzma': '5.8.2',
   #         'zstd': '1.5.7',
   #     },
   #     'tls': {
   #         'mbedtls': 'mbed TLS 2.28.10',
   #         'openssl_compat': False,
   #     },
   #     'http': {
   #         'switch_curl': True,
   #         'version': 'libcurl/7.69.1 ...',
   #     },
   # }

The exact version strings depend on the installed devkitPro packages.

``switch_ssl`` is not CPython's OpenSSL-backed ``ssl`` module.  mbedTLS is not
an OpenSSL ABI/API replacement, so this fork does not build CPython's
OpenSSL-specific ``_ssl`` or ``_hashlib`` modules against mbedTLS.  Hashing is
provided by CPython's built-in HACL-backed digest modules such as ``_md5``,
``_sha1``, ``_sha2``, ``_sha3``, ``_blake2``, and ``_hmac``.

``switch_curl``
---------------

``switch_curl`` is the Python-level integration point for curl-backed HTTP
support on Switch.

.. code-block:: python

   import switch_curl

   switch_curl.available()
   # True when switch-curl was detected at build time.

   switch_curl.backends()
   # Same backend report as switch_ssl.backends().

The module also reserves a requests adapter hook:

.. code-block:: python

   import switch_curl

   session = switch_curl.install_requests_adapter()

The adapter currently requires a low-level ``_switch_curl`` binding.  If that
binding is not compiled into the runtime, ``install_requests_adapter()`` raises
``RuntimeError`` with a clear message.

Compression Modules
-------------------

The Switch build enables these standard Python compression modules when the
matching devkitPro portlibs are installed:

* ``zlib`` from ``switch-zlib``
* ``_lzma`` / ``lzma`` from ``switch-liblzma``
* ``_zstd`` / ``compression.zstd`` from ``switch-libzstd``

These modules are built statically into the interpreter, not loaded from shared
extension files at runtime.

Static Extension Policy
-----------------------

Nintendo Switch homebrew does not use CPython's normal shared-extension module
workflow.  The Switch port sets ``MODULE_BUILDTYPE=static`` and links supported
C extension modules into ``libpython3.14.a`` and the embedding executable.

When adding a Switch-specific C module, build it as a static module through the
Switch portlibs build helper or the generated ``Modules/Setup.local``.

Native extension files must not be staged into RomFS.  RomFS is readable but is
not an executable-code deployment target for this port.  Extension modules that
would normally be distributed as wheels should be linked into the application
or packaged through a title layout that supports executable content.

Build Helper
------------

The fork-specific build helper is:

.. code-block:: sh

   DEVKITPRO=/opt/devkitpro \
   DEVKITA64=/opt/devkitpro/devkitA64 \
   PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH \
   Platforms/NX/build-portlibs.sh

It installs or stages:

.. code-block:: text

   include/python3.14/
   lib/python3.14/config-3.14/
   lib/pkgconfig/python-3.14-embed.pc
   share/python-switch/romfs/python/

The embed pkg-config file includes the static dependency closure required by
``libpython3.14.a`` on Switch, including the bundled HACL, expat, and mpdecimal
archives plus devkitPro portlibs such as curl, zlib, liblzma, zstd, mbedTLS,
and libnx when available.

Use ``--destdir`` to stage without writing directly to ``$DEVKITPRO``:

.. code-block:: sh

   Platforms/NX/build-portlibs.sh --destdir /tmp/python-switch-stage

Examples
--------

The repository includes complete Nintendo Switch homebrew examples in
``Examples/NX``.  They embed CPython, initialize libnx console and romfs
support, run a Python script from ``romfs:/python_examples/demo.py``, and
demonstrate the fork-specific filesystem roots, compression modules,
``hashlib``, ``switch_ssl``, and ``switch_curl`` groundwork.

After installing or staging the portlibs runtime, build the Makefile example
with:

.. code-block:: sh

   cd Examples/NX/make
   make prepare-romfs
   make

For a staged install:

.. code-block:: sh

   make prepare-romfs PYTHON_PREFIX=/tmp/python-switch-stage/opt/devkitpro/portlibs/switch
   make PYTHON_PREFIX=/tmp/python-switch-stage/opt/devkitpro/portlibs/switch

The CMake example lives in ``Examples/NX/cmake``:

.. code-block:: sh

   /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
     -S Examples/NX/cmake \
     -B Examples/NX/cmake/build

The CMake helper stages RomFS into a temporary directory in the build tree.  It
copies the application's RomFS contents first, then overlays the installed
Python runtime at ``romfs:/python`` before creating the RomFS image used by the
NRO.
   /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
     --build Examples/NX/cmake/build
