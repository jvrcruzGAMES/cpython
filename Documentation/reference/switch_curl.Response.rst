switch_curl.Response
====================

``switch_curl.Response`` is the result object returned by ``switch_curl``
transfers.

Constructor
-----------

.. code-block:: python

   response = switch_curl.Response(
       url,
       status_code,
       headers={},
       content=b"",
       reason=None,
       elapsed=None,
       encoding=None,
   )

Attributes
----------

``url``
   Effective response URL.

``status_code``
   HTTP status code.

``headers``
   Lowercase response header mapping.

``content``
   Response body bytes.

``reason``
   Optional status reason.

``elapsed``
   Optional transfer time in seconds.

``encoding``
   Text encoding.  If omitted, it is inferred from ``Content-Type`` and
   defaults to ``"utf-8"``.

Properties And Methods
----------------------

``text``
   ``content`` decoded with ``encoding`` and replacement error handling.

``ok``
   ``True`` for status codes from 200 through 399.

``json(**kwargs)``
   Decode ``text`` as JSON.

``raise_for_status()``
   Raise ``SwitchCurlError`` for non-``ok`` responses.
