switch_curl.Curl
================

``switch_curl.Curl`` is a small curl-easy style handle used by the
``switch_curl`` module.

Constructor
-----------

.. code-block:: python

   curl = switch_curl.Curl()

Attributes
----------

``options``
   Mapping of ``CURLOPT_*`` option constants to values.

``info``
   Mapping populated after ``perform()`` with ``CURLINFO_*`` values.

``response``
   The most recent :doc:`switch_curl.Response` or ``None``.

Methods
-------

``setopt(option, value)``
   Store a curl option for the next transfer.

``getinfo(info)``
   Return a stored info value such as ``CURLINFO_RESPONSE_CODE``.

``perform()``
   Execute the transfer and return :doc:`switch_curl.Response`.  Requires the
   native ``_switch_curl`` binding and curl integration opt-in.

``reset()``
   Clear options, info, and the last response.

``close()``
   Alias for ``reset()``.
