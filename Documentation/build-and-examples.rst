Build And Examples
==================

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

Use ``--destdir`` to stage without writing directly to ``$DEVKITPRO``:

.. code-block:: sh

   Platforms/NX/build-portlibs.sh --destdir /tmp/python-switch-stage

Examples
--------

The repository includes complete Nintendo Switch homebrew examples in
``Examples/NX``.  They embed CPython, initialize libnx console and RomFS
support, run ``romfs:/python_examples/demo.py``, and demonstrate the
fork-specific filesystem roots, compression modules, ``hashlib``,
``switch_support``, ``switch_ssl``, ``switch_curl``, ``switch_input``,
bundled packages, and linked native extension payloads.

Build the Make example:

.. code-block:: sh

   cd Examples/NX/make
   make

For a staged install:

.. code-block:: sh

   make PYTHON_PREFIX=/tmp/python-switch-stage/opt/devkitpro/portlibs/switch

Build the CMake example:

.. code-block:: sh

   /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
     -S Examples/NX/cmake \
     -B Examples/NX/cmake/build

   /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake \
     --build Examples/NX/cmake/build

The CMake helper stages RomFS into a temporary directory in the build tree,
copies the application's RomFS contents, overlays the installed Python runtime
at ``romfs:/python``, removes stale bytecode caches, and creates the RomFS
image used by the NRO.
