switch_curl.SwitchCurlError
===========================

``SwitchCurlError`` is the base exception for high-level ``switch_curl``
errors.

It subclasses ``RuntimeError`` and is raised for failed HTTP status checks and
unsupported native response shapes.
