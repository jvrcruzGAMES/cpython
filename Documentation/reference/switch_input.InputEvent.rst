switch_input.InputEvent
=======================

``InputEvent`` is a named tuple with the shape used by the third-party
``inputs`` package:

.. code-block:: python

   InputEvent(ev_type, code, state)

Fields
------

``ev_type``
   Event type, usually ``"Key"`` or ``"Absolute"``.

``code``
   Symbolic event code such as ``"BTN_A"``, ``"ABS_X"``, or ``"ABS_RY"``.

``state``
   Integer event value.  Buttons use ``1`` for press and ``0`` for release.
