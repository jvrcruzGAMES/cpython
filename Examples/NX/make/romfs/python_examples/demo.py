import hashlib
import lzma
import os
import posixpath
import sys
import zlib

import switch_curl
import switch_ssl


def section(name):
    print()
    print("==", name, "==")


def show(name, value):
    print(f"{name}: {value!r}")


def filesystem_examples():
    section("Switch filesystem roots")
    paths = [
        "romfs:",
        "romfs:/",
        "romfs:/python_examples/demo.py",
        "sdmc:",
        "sdmc:/switch/python",
    ]
    for path in paths:
        show(f"isabs({path})", posixpath.isabs(path))

    show("join romfs:", posixpath.join("romfs:", "python_examples/demo.py"))
    show("join romfs absolute", posixpath.join("romfs:/python", "/site.py"))
    show("splitroot sdmc", posixpath.splitroot("sdmc:/switch/python"))
    show("bytes join", posixpath.join(b"romfs:", b"python_examples/demo.py"))
    show("exists demo", os.path.exists("romfs:/python_examples/demo.py"))


def compression_examples():
    section("Compression")
    payload = b"CPython on Nintendo Switch" * 8

    zlib_data = zlib.compress(payload)
    show("zlib roundtrip", zlib.decompress(zlib_data) == payload)

    lzma_data = lzma.compress(payload)
    show("lzma roundtrip", lzma.decompress(lzma_data) == payload)

    try:
        import compression.zstd as zstd
        zstd_data = zstd.compress(payload)
        zstd_roundtrip = zstd.decompress(zstd_data) == payload
    except ImportError:
        import _zstd
        compressor = _zstd.ZstdCompressor()
        zstd_data = compressor.compress(payload, _zstd.ZstdCompressor.FLUSH_FRAME)
        decompressor = _zstd.ZstdDecompressor()
        zstd_roundtrip = decompressor.decompress(zstd_data) == payload
    show("zstd roundtrip", zstd_roundtrip)


def hashlib_examples():
    section("Hashlib")
    payload = b"switch-python"
    show("sha256", hashlib.sha256(payload).hexdigest())
    show("blake2b", hashlib.blake2b(payload, digest_size=16).hexdigest())


def backend_examples():
    section("Switch backends")
    backends = switch_ssl.backends()
    show("switch_ssl.backends", backends)
    show("switch_curl.available", switch_curl.available())
    show("switch_curl.backends", switch_curl.backends())


def requests_groundwork_example():
    section("Requests curl adapter groundwork")
    try:
        switch_curl.install_requests_adapter()
    except Exception as exc:
        show("adapter status", f"{type(exc).__name__}: {exc}")
    else:
        show("adapter status", "installed")


def main():
    print("CPython Nintendo Switch feature examples")
    print(sys.version)
    filesystem_examples()
    compression_examples()
    hashlib_examples()
    backend_examples()
    requests_groundwork_example()


if __name__ == "__main__":
    main()
