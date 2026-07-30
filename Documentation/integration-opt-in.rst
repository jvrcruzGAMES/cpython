Runtime Integration Opt-In
==========================

Switch-specific controls that can affect application lifecycle, TLS, HTTP, or
controller input are explicit opt-ins.  This keeps embedded applications in
control of which Python code can use host services.

``switch_support``
------------------

``switch_support`` is the central module for environment and backend
information:

.. code-block:: python

   import switch_support

   switch_support.environment()
   switch_support.backends()
   switch_support.model()
   switch_support.firmware_version()

If a Switch system service is not available, helpers return ``None`` or a
mapping with a libnx ``result`` code rather than raising during normal probing.

``sys.switch``
--------------

Direct NX control through ``sys`` is disabled by default.  A host NRO can
enable it after Python initialization:

.. code-block:: c

   #include <Python.h>
   #include <switch_support.h>

   PyStatus status = Py_InitializeFromConfig(&config);
   if (!PyStatus_Exception(status)) {
       if (PySwitch_EnableSysIntegration() < 0) {
           PyErr_Print();
       }
   }

Once enabled, Python code can use ``sys.switch``:

.. code-block:: python

   import sys

   sys.switch.is_switch()
   sys.switch.model
   sys.switch.firmware
   sys.switch.console_update()
   sys.switch.sleep(16_666_667)
   sys.switch.request_exit()

TLS, Curl, And Input Flags
--------------------------

``switch_ssl``, ``switch_curl``, and ``switch_input`` are compiled in when
their dependencies are available, but active TLS, HTTP, controller polling, and
software keyboard operations are disabled until explicitly enabled:

.. code-block:: c

   PySwitch_EnableSSLIntegration();
   PySwitch_EnableCurlIntegration();
   PySwitch_EnableInputIntegration();

``switch_input`` is intentionally controlled by the native host application
rather than by the Python script.  Until the host calls
``PySwitch_EnableInputIntegration()``, ``switch_input.available()`` returns
``False`` and calls such as ``switch_input.configure()``,
``switch_input.update()``, and ``switch_input.prompt_keyboard()`` report that
the backend is disabled.

Python code can inspect the host policy with ``switch_support`` helpers, but
cannot enable or disable integrations itself:

.. code-block:: python

   import switch_support

   switch_support.is_ssl_enabled()
   switch_support.is_curl_enabled()
   switch_support.is_input_enabled()
   switch_support.is_enabled("switch_curl")

The short probe helpers are:

* ``is_sys_enabled()``
* ``is_ssl_enabled()``
* ``is_curl_enabled()``
* ``is_input_enabled()``
* ``is_switch_input_enabled()``
* ``is_enabled(name)``
