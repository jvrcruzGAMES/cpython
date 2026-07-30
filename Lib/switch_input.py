"""Nintendo Switch controller input helpers.

The low-level backend is enabled by the Switch portlibs build configuration.
When present, this module exposes direct polling helpers plus a small wrapper
compatible with the shape of the third-party ``inputs`` package.
"""

from __future__ import annotations

from collections import namedtuple
from dataclasses import dataclass
from enum import IntEnum
from typing import Any

try:
    import _switch_input
except ImportError:  # pragma: no cover - expected when disabled by build config.
    _switch_input = None


InputEvent = namedtuple("InputEvent", "ev_type code state")


class SwitchInputNotAvailableError(RuntimeError):
    """Raised when the native Switch input backend is not compiled in."""


class KeyboardType(IntEnum):
    """libnx software keyboard layouts accepted by ``prompt_keyboard()``."""

    NORMAL = getattr(_switch_input, "KEYBOARD_TYPE_NORMAL", 0)
    NUMPAD = getattr(_switch_input, "KEYBOARD_TYPE_NUMPAD", 1)
    QWERTY = getattr(_switch_input, "KEYBOARD_TYPE_QWERTY", 2)
    LATIN = getattr(_switch_input, "KEYBOARD_TYPE_LATIN", 4)
    ZH_HANS = getattr(_switch_input, "KEYBOARD_TYPE_ZH_HANS", 5)
    ZH_HANT = getattr(_switch_input, "KEYBOARD_TYPE_ZH_HANT", 6)
    KOREAN = getattr(_switch_input, "KEYBOARD_TYPE_KOREAN", 7)
    ALL = getattr(_switch_input, "KEYBOARD_TYPE_ALL", 8)


def _require_backend():
    if _switch_input is None:
        raise SwitchInputNotAvailableError(
            "switch_input is not available in this build"
        )
    if not _switch_input.enabled():
        raise SwitchInputNotAvailableError(
            "switch_input is disabled; the host NRO must call "
            "PySwitch_EnableInputIntegration()"
        )
    return _switch_input


BUTTONS = getattr(_switch_input, "BUTTONS", {})
STYLE_STANDARD = getattr(_switch_input, "STYLE_STANDARD", 0)
STYLE_FULL = getattr(_switch_input, "STYLE_FULL", 0)


def available() -> bool:
    """Return whether the native Switch input backend is available and enabled."""
    return _switch_input is not None and _switch_input.enabled()


def enabled() -> bool:
    """Return whether the host NRO enabled Switch input integration."""
    return available()


def configure(max_players: int = 1, style_set: int = STYLE_STANDARD) -> None:
    """Configure libnx controller input and initialize the default pad."""
    _require_backend().configure(max_players=max_players, style_set=style_set)


def update() -> dict[str, Any]:
    """Poll controller input and return a state mapping."""
    return _require_backend().update()


def get_buttons() -> int:
    """Return the currently pressed button bitmask."""
    return _require_backend().get_buttons()


def get_buttons_down() -> int:
    """Return the newly pressed button bitmask."""
    return _require_backend().get_buttons_down()


def get_buttons_up() -> int:
    """Return the newly released button bitmask."""
    return _require_backend().get_buttons_up()


def is_connected() -> bool:
    """Return whether the default controller input source is connected."""
    return _require_backend().is_connected()


def button_names(mask: int) -> list[str]:
    """Return symbolic button names present in *mask*."""
    return list(_require_backend().button_names(mask))


def prompt_keyboard(
    prompt: str = "",
    *,
    keyboard_type: KeyboardType | int = KeyboardType.QWERTY,
    initial_text: str = "",
    max_length: int = 500,
    password: bool = False,
) -> str | None:
    """Open the Switch software keyboard and return entered text.

    ``None`` is returned when the keyboard applet is cancelled.  Set
    ``password=True`` to hide the typed characters.
    """
    return _require_backend().prompt_keyboard(
        prompt=prompt,
        keyboard_type=int(keyboard_type),
        initial_text=initial_text,
        max_length=max_length,
        password=password,
    )


def _events_from_state(state: dict[str, Any]) -> list[InputEvent]:
    events: list[InputEvent] = []
    for name in state["button_down_names"]:
        events.append(InputEvent("Key", f"BTN_{name}", 1))
    for name in state["button_up_names"]:
        events.append(InputEvent("Key", f"BTN_{name}", 0))

    left_x, left_y = state["left_stick"]
    right_x, right_y = state["right_stick"]
    events.extend(
        [
            InputEvent("Absolute", "ABS_X", left_x),
            InputEvent("Absolute", "ABS_Y", left_y),
            InputEvent("Absolute", "ABS_RX", right_x),
            InputEvent("Absolute", "ABS_RY", right_y),
        ]
    )
    return events


def get_gamepad() -> list[InputEvent]:
    """Return controller events using the third-party ``inputs`` package shape."""
    return _events_from_state(update())


@dataclass
class GamePad:
    """Minimal ``inputs``-style gamepad wrapper around the default NX pad."""

    name: str = "Nintendo Switch Controller"
    path: str = "switch://pad/default"
    phys: str = "switch"
    uniq: str = "default"

    def read(self) -> list[InputEvent]:
        return get_gamepad()


class _Devices:
    @property
    def gamepads(self) -> list[GamePad]:
        if not available():
            return []
        return [GamePad()]


devices = _Devices()
