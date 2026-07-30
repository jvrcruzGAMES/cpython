switch_curl.CurlNotAvailableError
=================================

``CurlNotAvailableError`` is raised when a transfer requires curl support that
is unavailable or disabled.

It subclasses :doc:`switch_curl.SwitchCurlError`.

Common causes are:

* devkitPro ``switch-curl`` was not available when CPython was built;
* the native ``_switch_curl`` binding is not compiled in;
* the host application has not enabled curl integration.
