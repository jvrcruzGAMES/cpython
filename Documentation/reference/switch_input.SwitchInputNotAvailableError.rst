switch_input.SwitchInputNotAvailableError
=========================================

``SwitchInputNotAvailableError`` is raised when code tries to use controller
polling or software keyboard helpers but the native ``_switch_input`` backend
is unavailable or the host NRO has not enabled input integration.

It subclasses ``RuntimeError``.

Host applications enable the backend at runtime by calling
``PySwitch_EnableInputIntegration()`` from native code.
