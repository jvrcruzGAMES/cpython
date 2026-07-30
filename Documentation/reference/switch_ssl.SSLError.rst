switch_ssl.SSLError
===================

``switch_ssl.SSLError`` is the ``switch_ssl`` module's TLS exception class.

It subclasses ``OSError`` and is raised for TLS operations that cannot be
completed by the current mbedTLS-backed Switch implementation, including the
reserved ``SSLContext.wrap_socket()`` path.
