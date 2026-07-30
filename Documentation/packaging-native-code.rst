Packaging Native Code
=====================

RomFS is readable data storage.  It should not be used for executable native
extension payloads in this fork.

For a Switch-built wheel that contains a native extension, split the package:

* Pure Python files, ``.dist-info`` metadata, certificates, and other data go
  into site-packages, usually ``romfs:/python_site``.
* Native extension code is built with devkitA64 and linked into the host NRO,
  or packaged in another executable title layout.
* The host app registers each linked extension with
  ``PyImport_AppendInittab()`` before ``Py_InitializeFromConfig()``.

Example:

.. code-block:: c

   PyMODINIT_FUNC PyInit__nx_native_demo(void);

   if (PyImport_AppendInittab("_nx_native_demo", PyInit__nx_native_demo) < 0) {
       /* handle registration failure */
   }

The extension can then be imported normally:

.. code-block:: python

   import _nx_native_demo
   _nx_native_demo.add(20, 22)

The Make and CMake example projects reject native package payloads from RomFS
and show how to link a native module into the NRO.
