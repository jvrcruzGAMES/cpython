Nintendo Switch Fork APIs
=========================

This documentation covers APIs and behavior that are specific to this CPython
fork.  For normal Python language and standard-library documentation, use the
upstream Python 3.14 documentation.

.. toctree::
   :maxdepth: 2
   :caption: Topics

   overview
   filesystems
   embedding-runtime
   integration-opt-in
   networking
   input
   packaging-native-code
   build-and-examples
   reference/index

Quick Links
-----------

* :doc:`filesystems` documents ``romfs:`` and ``sdmc:`` path support.
* :doc:`embedding-runtime` documents the runtime prefix, console streams, and
  ``input()`` behavior.
* :doc:`integration-opt-in` documents ``switch_support`` and host opt-in APIs.
* :doc:`networking` documents ``switch_ssl`` and ``switch_curl``.
* :doc:`input` documents ``switch_input`` controller polling.
* :doc:`packaging-native-code` documents why native extensions are linked into
  the NRO instead of loaded from RomFS.
* :doc:`reference/index` contains single-page references for classes and
  class-like objects added or modified by this fork.
