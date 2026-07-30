    Controller Input
================

``switch_input`` provides Nintendo Switch controller polling helpers.  The
native libnx backend is built into the Switch portlibs, but it is disabled at
runtime until the host NRO opts in.

Enable it from the native application after initializing Python and before
running Python code that imports or uses ``switch_input``:

.. code-block:: c

   #include <switch_support.h>

   if (PySwitch_EnableInputIntegration() < 0) {
       /* handle Python exception */
   }

When the backend is not enabled, ``switch_input.available()`` returns
``False`` and polling helpers raise ``SwitchInputNotAvailableError``.

Direct Polling
--------------

.. code-block:: python

   import switch_input

   if switch_input.available():
       switch_input.configure()
       state = switch_input.update()
       print(state["button_down_names"])
       print(state["left_stick"], state["right_stick"])

``update()`` returns a mapping with button bitmasks, symbolic button names,
stick tuples, and connection flags.

Software Keyboard
-----------------

``switch_input.prompt_keyboard()`` opens the libnx software keyboard applet
directly from Python:

.. code-block:: python

   import switch_input

   text = switch_input.prompt_keyboard(
       "Enter a player name",
       keyboard_type=switch_input.KeyboardType.QWERTY,
       initial_text="Player",
       max_length=32,
   )
   if text is None:
       print("cancelled")
   else:
       print(text)

Set ``password=True`` to request libnx's password-style keyboard, which hides
typed characters:

.. code-block:: python

   secret = switch_input.prompt_keyboard(
       "Enter password",
       keyboard_type=switch_input.KeyboardType.QWERTY,
       max_length=64,
       password=True,
   )

``KeyboardType`` mirrors libnx ``SwkbdType`` values exposed by the backend:
``NORMAL``, ``NUMPAD``, ``QWERTY``, ``LATIN``, ``ZH_HANS``, ``ZH_HANT``,
``KOREAN``, and ``ALL``.

The fork's built-in ``input()`` integration also uses the Switch software
keyboard when the host app has not bound stdin to a different source.  The
example applications include a keyboard test that prompts with ``input()``,
shows the returned text, and lets B return to the menu.

``inputs``-Style Wrapper
------------------------

The module also provides a small wrapper compatible with the shape of the
third-party ``inputs`` package:

.. code-block:: python

   import switch_input

   for event in switch_input.get_gamepad():
       print(event.ev_type, event.code, event.state)

   for pad in switch_input.devices.gamepads:
       for event in pad.read():
           print(event)

Button events use ``InputEvent("Key", "BTN_A", 1)`` or ``0`` for release.
Stick events use ``InputEvent("Absolute", "ABS_X", value)``,
``ABS_Y``, ``ABS_RX``, and ``ABS_RY``.
