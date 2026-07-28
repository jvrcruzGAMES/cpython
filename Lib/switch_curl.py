"""Nintendo Switch helpers for curl-backed HTTP integrations.

The Switch port keeps this module intentionally small.  It gives applications
and third-party packages a stable place to check whether this CPython build was
compiled with switch-curl support, and to install a requests transport adapter
once the low-level `_switch_curl` binding is available.
"""

from __future__ import annotations

try:
    import switch_ssl
except ImportError:  # pragma: no cover - only possible outside Switch builds.
    switch_ssl = None


def backends():
    """Return Switch networking/compression backend information."""
    if switch_ssl is None:
        return {
            "compression": {},
            "tls": {"mbedtls": None, "openssl_compat": False},
            "http": {"switch_curl": False, "version": None},
        }
    return switch_ssl.backends()


def available() -> bool:
    """Return true when the build has switch-curl support."""
    return bool(backends()["http"]["switch_curl"])


class SwitchCurlAdapter:
    """Placeholder requests adapter for the optional switch-curl binding."""

    def __init__(self, *args, **kwargs):
        try:
            import _switch_curl  # noqa: F401
        except ImportError as exc:
            raise RuntimeError(
                "switch-curl support is not available in this Python build"
            ) from exc


def install_requests_adapter(session=None):
    """Install the Switch curl adapter into a requests Session.

    The adapter surface is intentionally present before `_switch_curl` ships so
    package authors can probe it without special-casing the platform.
    """
    if session is None:
        import requests

        session = requests.Session()
    adapter = SwitchCurlAdapter()
    session.mount("http://", adapter)
    session.mount("https://", adapter)
    return session
