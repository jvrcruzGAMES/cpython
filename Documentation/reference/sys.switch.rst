sys.switch
==========

``sys.switch`` is a class-like module object attached to ``sys`` when a host
application enables sys integration.

It is disabled by default.  Hosts enable it from native code with
``PySwitch_EnableSysIntegration()``.

Attributes
----------

``enabled``
   Always ``True`` while the object is installed.

``firmware``
   Firmware snapshot mapping or ``None``.

``model``
   Hardware model mapping.

Methods
-------

``is_switch()``
   Return ``True`` on this port.

``applet_main_loop()``
   Call libnx ``appletMainLoop()``.

``request_exit()``
   Request applet exit and return a mapping with ``ok`` and ``result``.

``console_update()``
   Flush the active libnx console.

``sleep(nanoseconds)``
   Sleep with ``svcSleepThread()``.
