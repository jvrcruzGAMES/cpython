Python for Nintendo Switch homebrew
===================================

This repository is a fork of CPython 3.14 for Nintendo Switch homebrew.
It exists to make Python practical as an embedded runtime in ``.nro``
applications built with devkitPro, devkitA64, and libnx.

The goal is not to replace upstream CPython.  The goal is to keep a small,
maintained Switch port that can be installed into devkitPro portlibs, linked
as ``libpython3.14.a``, and loaded from a homebrew application's ``romfs``.

Why This Fork Exists
--------------------

Upstream CPython targets desktop, server, mobile, and several embedded-style
platforms, but Nintendo Switch homebrew has a different runtime shape:

* applications are statically linked with devkitA64/libnx;
* shared extension modules are not a normal deployment mechanism;
* the standard library needs to live in ``romfs:/`` or ``sdmc:/``;
* many POSIX process, terminal, user database, and dynamic loading APIs are
  unavailable or unsuitable;
* embedders need predictable headers, pkg-config files, and a staged runtime
  tree under ``$DEVKITPRO/portlibs/switch``.

This fork carries the Switch-specific configure, path, filesystem, and build
changes needed for that workflow while staying close to CPython 3.14.

Supported Filesystems
---------------------

The Switch port treats these roots as first-class Python paths:

* ``romfs:``
* ``romfs:/``
* ``sdmc:``
* ``sdmc:/``

Both rooted forms are supported so embedders can use natural libnx paths while
still relying on Python path operations such as ``join()``, ``isabs()``,
``splitroot()``, ``normpath()``, and import path resolution.

Console Integration
-------------------

When an embedded Switch application initializes the libnx console and does not
install custom Python streams, this fork routes ``sys.stdout`` and
``sys.stderr`` to that console by default.  Python ``input()`` opens the libnx
software keyboard.  Embedders can still replace ``sys.stdin``, ``sys.stdout``,
and ``sys.stderr`` with application-specific objects after initialization.

NX control through ``sys`` is disabled by default.  A host NRO can opt in by
including ``switch_support.h`` and calling ``PySwitch_EnableSysIntegration()``
after ``Py_InitializeFromConfig()``.  When enabled, Python code gets a
``sys.switch`` namespace with helpers such as ``applet_main_loop()``,
``request_exit()``, ``console_update()``, and ``sleep()``.

``switch_ssl`` TLS helpers and ``switch_curl`` HTTP transfers are also disabled
by default at runtime.  A host NRO can opt in with
``PySwitch_EnableSSLIntegration()`` and ``PySwitch_EnableCurlIntegration()``,
or Python code can call the matching ``switch_support.enable_*_integration()``
helpers when the host allows that policy.

Build And Install
-----------------

The primary build entry point is:

.. code-block:: sh

   DEVKITPRO=/opt/devkitpro \
   DEVKITA64=/opt/devkitpro/devkitA64 \
   PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH \
   Platforms/NX/build-portlibs.sh

By default this installs into:

.. code-block:: text

   $DEVKITPRO/portlibs/switch

If that directory is not writable, stage the install instead:

.. code-block:: sh

   Platforms/NX/build-portlibs.sh --destdir /tmp/python-switch-stage

The installed/staged layout contains:

.. code-block:: text

   include/python3.14/                 C API headers
   lib/python3.14/config-3.14/         static lib and build config
   lib/pkgconfig/python-3.14-embed.pc  embedding pkg-config file
   share/python-switch/romfs/python/   runtime tree for app romfs

More detailed build and embedding notes are in ``Platforms/NX/README.md``.
Fork-specific APIs and behavior are documented in ``DOCUMENTATION.rst``.

Embedded Module Set
-------------------

The Switch build uses static built-in extension modules.  These are the
compatible modules this fork currently builds and intends to keep maintained:

* ``array``
* ``atexit``
* ``binascii``
* ``cmath``
* ``errno``
* ``faulthandler``
* ``itertools``
* ``math``
* ``posix``
* ``pyexpat``
* ``select``
* ``signal``
* ``symtable``
* ``time``
* ``unicodedata``
* ``_abc``
* ``_asyncio``
* ``_bisect``
* ``_blake2``
* ``_codecs``
* ``_codecs_cn``
* ``_codecs_hk``
* ``_codecs_iso2022``
* ``_codecs_jp``
* ``_codecs_kr``
* ``_codecs_tw``
* ``_collections``
* ``_csv``
* ``_datetime``
* ``_decimal``
* ``_elementtree``
* ``_functools``
* ``_heapq``
* ``_hmac``
* ``_interpchannels``
* ``_interpqueues``
* ``_interpreters``
* ``_io``
* ``_json``
* ``_locale``
* ``_lsprof``
* ``_lzma``
* ``_md5``
* ``_multibytecodec``
* ``_opcode``
* ``_operator``
* ``_pickle``
* ``_queue``
* ``_random``
* ``_sha1``
* ``_sha2``
* ``_sha3``
* ``_socket``
* ``_sre``
* ``_stat``
* ``_statistics``
* ``_struct``
* ``_switch_curl`` (when ``switch-curl`` is available)
* ``_suggestions``
* ``_sysconfig``
* ``_thread``
* ``_tracemalloc``
* ``_types``
* ``_typing``
* ``_weakref``
* ``_zstd``
* ``_zoneinfo``
* ``switch_input`` (Python wrapper; native backend is optional)
* ``switch_curl``
* ``switch_ssl``
* ``switch_support``
* ``zlib``

Compression support is linked against devkitPro's ``switch-zlib``,
``switch-liblzma``, and ``switch-zstd`` portlibs.  ``hashlib`` is supported for
the embedded-safe digest algorithms through CPython's built-in HACL-backed
modules such as ``_md5``, ``_sha1``, ``_sha2``, ``_sha3``, ``_blake2``, and
``_hmac``.

The ``switch_support`` module reports Switch environment and backend
information, including firmware version, product model, linked compression
libraries, mbedTLS, and optional switch-curl.

The ``switch_ssl`` module exposes mbedTLS-backed helpers such as
``RAND_bytes()``, ``RAND_status()``, ``mbedtls_version()``,
``mbedtls_strerror()``, and an ``SSLContext`` object for TLS configuration.
Active TLS operations require the runtime SSL opt-in.

The ``switch_curl`` Python module is the stable integration point for packages
that want to use curl-backed HTTP on Switch.  It provides curl-like handles
(``Curl.setopt()``, ``Curl.perform()``, ``Curl.getinfo()``), high-level
``request()`` / ``get()`` / ``post()`` helpers, a small ``Session`` object, and
requests adapter groundwork.  When devkitPro's ``switch-curl`` portlib is
available, the build links the native ``_switch_curl`` backend automatically.
HTTP transfers and the requests adapter require the runtime curl opt-in.  The
native binding initializes the libnx socket service lazily before the first
transfer.

The ``switch_input`` Python module exposes controller polling helpers.  Its
native libnx backend is disabled by default and can be built with
``Platforms/NX/build-portlibs.sh --enable-switch-input`` or ``SWITCH_INPUT=1``.
The module also provides a small wrapper compatible with the shape of the
third-party ``inputs`` package.

Unsupported Or Deferred Modules
-------------------------------

Some CPython modules are intentionally disabled for Switch because they depend
on APIs that are not available or not appropriate for a static homebrew
runtime.  This includes dynamic loading, subprocess and multiprocessing
support, POSIX shared memory, terminal UI modules, user/group database
modules, ``mmap``, ``ctypes``, sqlite, OpenSSL-backed ``ssl``/``_hashlib``,
``ensurepip``, and the CPython test extension modules.

``_bz2`` remains disabled until a Switch bzip2 portlib is wired in.  CPython's
OpenSSL-specific ``_ssl`` and ``_hashlib`` modules are not built against
mbedTLS because mbedTLS is not an OpenSSL ABI/API replacement; Switch-specific
TLS/HTTP integrations should use the mbedTLS and curl groundwork exposed by
``switch_ssl`` and ``switch_curl``.

Upstream CPython
----------------

This project is based on CPython and keeps the original license terms.  For
general Python documentation, language reference material, and upstream
development information, see:

* https://www.python.org/
* https://docs.python.org/3.14/
* https://github.com/python/cpython

Copyright and License
---------------------

This fork preserves CPython's copyright and license information.  See
``LICENSE`` and the files under ``Doc/license.rst`` for the full upstream
license text and notices.
