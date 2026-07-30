Embedding Runtime
=================

Runtime Prefix
--------------

The portlibs build configures CPython to load its standard library from:

.. code-block:: text

   romfs:/python/lib/python3.14

Applications should copy the staged runtime tree into their homebrew RomFS:

.. code-block:: text

   $DEVKITPRO/portlibs/switch/share/python-switch/romfs/python

The runtime prefix can be changed when building:

.. code-block:: sh

   Platforms/NX/build-portlibs.sh --runtime-prefix sdmc:/python

Console Streams
---------------

On Switch, CPython installs console-backed standard streams by default when
the embedding application has not provided custom Python ``sys.stdin``,
``sys.stdout``, or ``sys.stderr`` objects.

``sys.stdout`` and ``sys.stderr`` write to the active libnx console:

.. code-block:: python

   print("Hello from Python on Switch", flush=True)

``input()`` reads through libnx's software keyboard applet.  If the user
cancels the keyboard, ``input()`` receives end-of-file behavior.

Applications that need custom logging, sockets, files, overlays, or a custom
keyboard UI can replace ``sys.stdin``, ``sys.stdout``, and ``sys.stderr`` after
initialization.

Site-Packages
-------------

Host applications select third-party package locations by appending the desired
directory to ``PyConfig.module_search_paths`` before calling
``Py_InitializeFromConfig()``:

.. code-block:: c

   config.module_search_paths_set = 1;
   append_bytes_path(&config.module_search_paths, "romfs:/python/lib/python3.14");
   append_bytes_path(&config.module_search_paths, "romfs:/python_site");

The example projects expose this path as ``PYTHON_SITE_PACKAGES`` for Make and
``SWITCH_PYTHON_SITE_PACKAGES`` for CMake.
