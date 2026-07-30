switch_curl.SwitchCurlAdapter
=============================

``SwitchCurlAdapter`` is the transport adapter installed into a
``requests.Session`` by ``switch_curl.install_requests_adapter()``.

Methods
-------

``send(prepared_request, **kwargs)``
   Convert a requests prepared request into a ``switch_curl`` transfer and
   return a ``requests.Response``.  The response encoding is copied from the
   ``switch_curl`` response and defaults to UTF-8.

``close()``
   Present for requests adapter compatibility.

Requirements
------------

The adapter requires the native ``_switch_curl`` binding and curl integration
opt-in.  If either is missing, construction or transfer raises
``CurlNotAvailableError``.
