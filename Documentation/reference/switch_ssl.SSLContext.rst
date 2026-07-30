switch_ssl.SSLContext
=====================

``switch_ssl.SSLContext`` is a mbedTLS-backed TLS configuration object.  It is
compatible with a subset of CPython's ``ssl.SSLContext`` surface.

Constructor
-----------

.. code-block:: python

   context = switch_ssl.SSLContext(protocol=switch_ssl.PROTOCOL_TLS_CLIENT)

Attributes
----------

``protocol``
   Protocol constant.

``verify_mode``
   One of ``CERT_NONE``, ``CERT_OPTIONAL``, or ``CERT_REQUIRED``.

``check_hostname``
   Boolean hostname checking flag.

``cafile``, ``capath``, ``cadata``
   Verification location values set by ``load_verify_locations()``.

Methods
-------

``load_verify_locations(cafile=None, capath=None, cadata=None)``
   Store certificate verification inputs.

``set_ciphers(ciphers)``
   Store a mbedTLS cipher configuration string.

``get_ciphers()``
   Return configured ciphers in a list-of-mappings shape.

``cert_store_stats()``
   Return zeroed certificate store counters.

``set_default_verify_paths()``
   Compatibility no-op.

``wrap_socket(sock, server_side=False, server_hostname=None)``
   Reserved for future socket integration.  Currently raises
   :doc:`switch_ssl.SSLError`.
