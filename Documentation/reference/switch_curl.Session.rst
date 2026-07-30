switch_curl.Session
===================

``switch_curl.Session`` is a small requests-like session object backed by
``switch_curl.request()``.

Attributes
----------

``headers``
   Default request headers.

``verify``
   TLS verification setting.  May be ``True``, ``False``, or a CA file path.

``follow_redirects``
   Whether requests follow redirects.

``timeout``
   Optional default timeout in seconds.

Methods
-------

``request(method, url, **kwargs)``
   Perform a request and return :doc:`switch_curl.Response`.

``get(url, **kwargs)``, ``post(url, **kwargs)``, ``put(url, **kwargs)``,
``patch(url, **kwargs)``, ``delete(url, **kwargs)``
   Convenience methods.

``close()``
   Present for requests-like compatibility.
