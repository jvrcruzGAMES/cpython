switch_input.GamePad
====================

``GamePad`` is a minimal ``inputs``-style gamepad wrapper around the default
Nintendo Switch controller source.

Attributes
----------

``name``
   ``"Nintendo Switch Controller"`` by default.

``path``
   ``"switch://pad/default"`` by default.

``phys``
   ``"switch"`` by default.

``uniq``
   ``"default"`` by default.

Methods
-------

``read()``
   Poll the default pad and return a list of :doc:`switch_input.InputEvent`.
