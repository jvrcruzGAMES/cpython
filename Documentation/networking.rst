Networking And TLS
==================

``switch_ssl``
--------------

``switch_ssl`` is a Switch-specific built-in module backed by mbedTLS.  It is
not CPython's OpenSSL-backed ``ssl`` module, but it provides a
compatibility-oriented subset for embedders and libraries that need TLS
configuration objects.

.. code-block:: python

   import switch_ssl

   switch_ssl.mbedtls_version()
   switch_ssl.RAND_status()
   switch_ssl.RAND_bytes(32)

   context = switch_ssl.create_default_context()
   context.verify_mode = switch_ssl.CERT_REQUIRED
   context.check_hostname = True
   context.load_verify_locations(cafile="romfs:/certs/ca.pem")

``SSLContext.wrap_socket()`` is reserved and currently raises
``switch_ssl.SSLError`` until Switch socket transport is wired through mbedTLS.

``switch_curl``
---------------

``switch_curl`` is the Python-level integration point for curl-backed HTTP on
Switch.  It can be imported even when the low-level ``_switch_curl`` binding is
not available, so libraries can probe support predictably.

.. code-block:: python

   import switch_curl

   curl = switch_curl.Curl()
   curl.setopt(switch_curl.CURLOPT_URL, "https://example.com/")
   curl.setopt(switch_curl.CURLOPT_FOLLOWLOCATION, True)
   response = curl.perform()
   print(response.status_code, response.text)

High-level helpers are also available:

.. code-block:: python

   response = switch_curl.get("https://example.com/")
   response.raise_for_status()
   print(response.text)

Requests Adapter
----------------

The module includes a requests adapter:

.. code-block:: python

   import switch_curl

   session = switch_curl.install_requests_adapter()
   response = session.get("http://example.com/", timeout=10)

The adapter uses the ``switch_curl`` transfer path and sets the resulting
``requests.Response`` encoding from HTTP headers, defaulting to UTF-8.

Socket Service
--------------

On Switch, the native ``_switch_curl`` binding calls
``socketInitializeDefault()`` before the first transfer and calls
``socketExit()`` when the module is cleaned up.  Hosts still need to opt in to
curl integration, but Python scripts do not need a separate socket bootstrap
for ``switch_curl`` transfers.
