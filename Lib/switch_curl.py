"""Nintendo Switch helpers for curl-backed HTTP integrations.

This module exposes a small Pythonic curl-style API.  The high-level objects
are available even when the optional low-level ``_switch_curl`` binding has not
been compiled yet, so applications and packages can probe for support and fail
with a useful error instead of importing different modules per platform.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Callable

try:
    import switch_support
except ImportError:  # pragma: no cover - only possible outside Switch builds.
    switch_support = None

try:
    import _switch_curl
except ImportError:  # pragma: no cover - normal until the C binding exists.
    _switch_curl = None


CURLOPT_URL = "URL"
CURLOPT_HTTPGET = "HTTPGET"
CURLOPT_POST = "POST"
CURLOPT_POSTFIELDS = "POSTFIELDS"
CURLOPT_CUSTOMREQUEST = "CUSTOMREQUEST"
CURLOPT_HTTPHEADER = "HTTPHEADER"
CURLOPT_ACCEPT_ENCODING = "ACCEPT_ENCODING"
CURLOPT_USERAGENT = "USERAGENT"
CURLOPT_TIMEOUT = "TIMEOUT"
CURLOPT_CONNECTTIMEOUT = "CONNECTTIMEOUT"
CURLOPT_FOLLOWLOCATION = "FOLLOWLOCATION"
CURLOPT_MAXREDIRS = "MAXREDIRS"
CURLOPT_WRITEFUNCTION = "WRITEFUNCTION"
CURLOPT_HEADERFUNCTION = "HEADERFUNCTION"
CURLOPT_CAINFO = "CAINFO"
CURLOPT_SSL_VERIFYPEER = "SSL_VERIFYPEER"
CURLOPT_SSL_VERIFYHOST = "SSL_VERIFYHOST"

CURLINFO_RESPONSE_CODE = "RESPONSE_CODE"
CURLINFO_EFFECTIVE_URL = "EFFECTIVE_URL"
CURLINFO_CONTENT_TYPE = "CONTENT_TYPE"
CURLINFO_TOTAL_TIME = "TOTAL_TIME"


class SwitchCurlError(RuntimeError):
    """Raised for Switch curl setup and transfer errors."""


class CurlNotAvailableError(SwitchCurlError):
    """Raised when a transfer needs the optional low-level curl binding."""


def _require_binding():
    if _switch_curl is None:
        raise CurlNotAvailableError(
            "switch-curl support is not available in this Python build"
        )
    if not enabled():
        raise CurlNotAvailableError(
            "switch-curl support is disabled; enable it with "
            "PySwitch_EnableCurlIntegration() from the host NRO"
        )
    return _switch_curl


def backends():
    """Return Switch networking/compression backend information."""
    if switch_support is None:
        return {
            "compression": {},
            "tls": {"mbedtls": None, "openssl_compat": False, "enabled": False},
            "http": {"switch_curl": False, "version": None, "enabled": False},
        }
    return switch_support.backends()


def available() -> bool:
    """Return true when the build has switch-curl support."""
    return bool(backends()["http"]["switch_curl"])


def enabled() -> bool:
    """Return true when the host app has enabled switch-curl transfers."""
    if switch_support is None:
        return False
    return bool(switch_support.curl_integration_enabled())


def version() -> str | None:
    """Return the linked libcurl version string, if available."""
    return backends()["http"]["version"]


def version_info() -> dict[str, Any]:
    """Return curl backend details in a stable Python mapping."""
    http = backends()["http"]
    return {
        "available": bool(http["switch_curl"]),
        "enabled": enabled(),
        "version": http["version"],
        "binding": _switch_curl is not None,
    }


def runtime_info() -> dict[str, Any]:
    """Return runtime state for the native curl/socket services."""
    if _switch_curl is None or not hasattr(_switch_curl, "runtime_info"):
        return {"socket_initialized": False, "curl_initialized": False}
    return dict(_switch_curl.runtime_info())


@dataclass
class Response:
    """Result returned by the high-level request helpers."""

    url: str
    status_code: int
    headers: dict[str, str] = field(default_factory=dict)
    content: bytes = b""
    reason: str | None = None
    elapsed: float | None = None
    encoding: str | None = None

    def __post_init__(self):
        if self.encoding is None:
            self.encoding = _encoding_from_headers(self.headers)

    def _content_type_parts(self) -> tuple[str, dict[str, str]]:
        content_type = self.headers.get("content-type", "")
        media_type, _, parameters = content_type.partition(";")
        parsed_parameters = {}
        for part in parameters.split(";"):
            key, _, value = part.strip().partition("=")
            if key and value:
                parsed_parameters[key.lower()] = value.strip("'\"")
        return media_type.strip().lower(), parsed_parameters

    @property
    def text(self) -> str:
        return self.content.decode(self.encoding or "utf-8", "replace")

    @property
    def ok(self) -> bool:
        return 200 <= self.status_code < 400

    def json(self, **kwargs):
        import json

        return json.loads(self.text, **kwargs)

    def raise_for_status(self):
        if not self.ok:
            raise SwitchCurlError(
                f"HTTP request failed with status {self.status_code}"
            )


def _encoding_from_headers(headers: dict[str, str]) -> str:
    content_type = headers.get("content-type", "")
    media_type, _, parameters = content_type.partition(";")
    for part in parameters.split(";"):
        key, _, value = part.strip().partition("=")
        if key.lower() == "charset" and value:
            return value.strip("'\"")
    return "utf-8"


def response_encoding(response: Any) -> str:
    """Return a response encoding, defaulting to UTF-8."""
    encoding = getattr(response, "encoding", None)
    if encoding:
        return encoding
    headers = getattr(response, "headers", {}) or {}
    if hasattr(headers, "items"):
        return _encoding_from_headers({str(k).lower(): str(v) for k, v in headers.items()})
    return "utf-8"


class Curl:
    """A curl-easy style handle.

    Options are set with ``setopt()`` using the ``CURLOPT_*`` constants above.
    ``perform()`` delegates to ``_switch_curl`` when that binding is available.
    """

    def __init__(self):
        self.options: dict[str, Any] = {}
        self.info: dict[str, Any] = {}
        self.response: Response | None = None

    def setopt(self, option: str, value: Any):
        self.options[option] = value

    def getinfo(self, info: str):
        return self.info.get(info)

    def reset(self):
        self.options.clear()
        self.info.clear()
        self.response = None

    def close(self):
        self.reset()

    def perform(self) -> Response:
        binding = _require_binding()
        if hasattr(binding, "perform"):
            result = binding.perform(dict(self.options))
        else:
            handle = binding.Curl()
            for option, value in self.options.items():
                handle.setopt(option, value)
            result = handle.perform()

        response = _coerce_response(result, self.options.get(CURLOPT_URL, ""))
        self.response = response
        self.info.update(
            {
                CURLINFO_RESPONSE_CODE: response.status_code,
                CURLINFO_EFFECTIVE_URL: response.url,
                CURLINFO_CONTENT_TYPE: response.headers.get("content-type"),
                CURLINFO_TOTAL_TIME: response.elapsed,
            }
        )

        writefunc = self.options.get(CURLOPT_WRITEFUNCTION)
        if writefunc is not None:
            writefunc(response.content)
        return response


def _coerce_response(result: Any, fallback_url: str) -> Response:
    if isinstance(result, Response):
        return result
    if isinstance(result, dict):
        return Response(
            url=result.get("url", fallback_url),
            status_code=int(result.get("status_code", result.get("code", 0))),
            headers={str(k).lower(): str(v) for k, v in result.get("headers", {}).items()},
            content=result.get("content", b""),
            reason=result.get("reason"),
            elapsed=result.get("elapsed"),
            encoding=result.get("encoding"),
        )
    if isinstance(result, (bytes, bytearray)):
        return Response(fallback_url, 0, content=bytes(result))
    raise SwitchCurlError(f"unsupported _switch_curl response type: {type(result).__name__}")


def request(
    method: str,
    url: str,
    *,
    headers: dict[str, str] | None = None,
    data: bytes | str | None = None,
    json: Any = None,
    timeout: float | None = None,
    follow_redirects: bool = True,
    verify: bool | str = True,
    write: Callable[[bytes], Any] | None = None,
) -> Response:
    """Perform a curl-backed HTTP request."""
    curl = Curl()
    curl.setopt(CURLOPT_URL, url)
    curl.setopt(CURLOPT_CUSTOMREQUEST, method.upper())
    curl.setopt(CURLOPT_FOLLOWLOCATION, bool(follow_redirects))
    curl.setopt(CURLOPT_ACCEPT_ENCODING, "")

    if timeout is not None:
        curl.setopt(CURLOPT_TIMEOUT, timeout)
    if headers:
        curl.setopt(CURLOPT_HTTPHEADER, [f"{k}: {v}" for k, v in headers.items()])
    if write is not None:
        curl.setopt(CURLOPT_WRITEFUNCTION, write)
    if isinstance(verify, str):
        curl.setopt(CURLOPT_CAINFO, verify)
    else:
        curl.setopt(CURLOPT_SSL_VERIFYPEER, bool(verify))
        curl.setopt(CURLOPT_SSL_VERIFYHOST, 2 if verify else 0)
    if json is not None:
        import json as json_module

        data = json_module.dumps(json).encode("utf-8")
        header_items = list((headers or {}).items())
        if not any(k.lower() == "content-type" for k, _ in header_items):
            header_items.append(("Content-Type", "application/json"))
        curl.setopt(CURLOPT_HTTPHEADER, [f"{k}: {v}" for k, v in header_items])
    if data is not None:
        if isinstance(data, str):
            data = data.encode()
        curl.setopt(CURLOPT_POSTFIELDS, data)
    if method.upper() == "GET":
        curl.setopt(CURLOPT_HTTPGET, True)
    elif method.upper() == "POST":
        curl.setopt(CURLOPT_POST, True)
    return curl.perform()


def get(url: str, **kwargs) -> Response:
    return request("GET", url, **kwargs)


def post(url: str, **kwargs) -> Response:
    return request("POST", url, **kwargs)


def put(url: str, **kwargs) -> Response:
    return request("PUT", url, **kwargs)


def patch(url: str, **kwargs) -> Response:
    return request("PATCH", url, **kwargs)


def delete(url: str, **kwargs) -> Response:
    return request("DELETE", url, **kwargs)


class Session:
    """Small requests-like session backed by ``Curl`` handles."""

    def __init__(self):
        self.headers: dict[str, str] = {}
        self.verify: bool | str = True
        self.follow_redirects = True
        self.timeout: float | None = None

    def request(self, method: str, url: str, **kwargs) -> Response:
        headers = dict(self.headers)
        headers.update(kwargs.pop("headers", {}) or {})
        kwargs.setdefault("headers", headers)
        kwargs.setdefault("verify", self.verify)
        kwargs.setdefault("follow_redirects", self.follow_redirects)
        if self.timeout is not None:
            kwargs.setdefault("timeout", self.timeout)
        return request(method, url, **kwargs)

    def get(self, url: str, **kwargs) -> Response:
        return self.request("GET", url, **kwargs)

    def post(self, url: str, **kwargs) -> Response:
        return self.request("POST", url, **kwargs)

    def put(self, url: str, **kwargs) -> Response:
        return self.request("PUT", url, **kwargs)

    def patch(self, url: str, **kwargs) -> Response:
        return self.request("PATCH", url, **kwargs)

    def delete(self, url: str, **kwargs) -> Response:
        return self.request("DELETE", url, **kwargs)

    def close(self):
        pass


class SwitchCurlAdapter:
    """Requests adapter for the optional switch-curl binding."""

    def __init__(self, *args, **kwargs):
        _require_binding()

    def send(self, prepared_request, **kwargs):
        response = request(
            prepared_request.method,
            prepared_request.url,
            headers=dict(prepared_request.headers),
            data=prepared_request.body,
            timeout=kwargs.get("timeout"),
            verify=kwargs.get("verify", True),
        )
        try:
            from requests import Response as RequestsResponse
        except ImportError as exc:  # pragma: no cover
            raise SwitchCurlError("requests is not available") from exc

        converted = RequestsResponse()
        converted.status_code = response.status_code
        converted.url = response.url
        converted._content = response.content
        converted.headers.update(response.headers)
        converted.encoding = response_encoding(response)
        converted.reason = response.reason
        converted.request = prepared_request
        return converted

    def close(self):
        pass


def install_requests_adapter(session=None):
    """Install the Switch curl adapter into a requests Session."""
    if session is None:
        import requests

        session = requests.Session()
    adapter = SwitchCurlAdapter()
    session.mount("http://", adapter)
    session.mount("https://", adapter)
    return session
