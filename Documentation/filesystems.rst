Filesystem Roots
================

The Switch port treats libnx filesystem roots as absolute Python paths:

* ``romfs:``
* ``romfs:/``
* ``sdmc:``
* ``sdmc:/``

Both root forms are accepted.  ``romfs:`` and ``romfs:/`` mean the same root,
and the same applies to ``sdmc:`` and ``sdmc:/``.

Examples:

.. code-block:: python

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

The import system can use these roots through ``PyConfig.module_search_paths``.
The examples add ``romfs:/python/lib/python3.14`` and a site-packages path such
as ``romfs:/python_site``.
