import hashlib
import contextlib
import io
import lzma
import os
import posixpath
import sys
import traceback
import zlib

import switch_curl
import switch_input
import switch_ssl
import switch_support
import requests
import _nx_native_demo


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
    backends = switch_support.backends()
    show("switch_support.backends", backends)
    show("switch_support.environment", switch_support.environment())
    show("switch_support.sys_integration_enabled", switch_support.sys_integration_enabled())
    show("switch_support.ssl_integration_enabled", switch_support.ssl_integration_enabled())
    show("switch_support.curl_integration_enabled", switch_support.curl_integration_enabled())
    show("switch_support.input_integration_enabled", switch_support.input_integration_enabled())
    show("switch_support.is_sys_enabled", switch_support.is_sys_enabled())
    show("switch_support.is_ssl_enabled", switch_support.is_ssl_enabled())
    show("switch_support.is_curl_enabled", switch_support.is_curl_enabled())
    show("switch_support.is_input_enabled", switch_support.is_input_enabled())
    show("switch_support.is_enabled('switch_curl')", switch_support.is_enabled("switch_curl"))
    show("sys.switch present", hasattr(sys, "switch"))
    if hasattr(sys, "switch"):
        show("sys.switch.model", sys.switch.model)
        show("sys.switch.firmware", sys.switch.firmware)
        sys.switch.console_update()
    show("switch_ssl.mbedtls_version", switch_ssl.mbedtls_version())
    show("switch_ssl.RAND_status", switch_ssl.RAND_status())
    show("switch_ssl.RAND_bytes length", len(switch_ssl.RAND_bytes(16)))
    ctx = switch_ssl.create_default_context()
    show("switch_ssl context verify_mode", ctx.verify_mode)
    show("switch_curl.available", switch_curl.available())
    show("switch_curl.enabled", switch_curl.enabled())
    show("switch_curl.version_info", switch_curl.version_info())
    show("switch_curl.runtime_info", switch_curl.runtime_info())
    curl = switch_curl.Curl()
    curl.setopt(switch_curl.CURLOPT_URL, "https://example.com/")
    show("switch_curl curl url", curl.options[switch_curl.CURLOPT_URL])


def requests_groundwork_example():
    section("Requests curl adapter groundwork")
    show("requests version", requests.__version__)
    show("requests package", requests.__file__)
    show("requests Session", type(requests.Session()).__name__)
    try:
        switch_curl.install_requests_adapter()
    except Exception as exc:
        show("adapter status", f"{type(exc).__name__}: {exc}")
    else:
        show("adapter status", "installed")


def requests_http_example():
    section("Requests adapter HTTP")
    session = switch_curl.install_requests_adapter()
    response = session.get("http://example.com/", timeout=10, verify=False)
    show("status_code", response.status_code)
    show("url", response.url)
    show("content-type", response.headers.get("content-type"))
    show("encoding", switch_curl.response_encoding(response))
    show("body prefix", response.text[:80].replace("\n", " "))


def switch_curl_http_example():
    section("switch_curl direct HTTP")
    response = switch_curl.get("http://example.com/", timeout=10, verify=False)
    show("status_code", response.status_code)
    show("url", response.url)
    show("content-type", response.headers.get("content-type"))
    show("encoding", switch_curl.response_encoding(response))
    show("elapsed", response.elapsed)
    show("body prefix", response.text[:80].replace("\n", " "))


def native_extension_example():
    section("Native extension linked in ExeFS")
    show("_nx_native_demo origin", _nx_native_demo.origin())
    show("_nx_native_demo.add", _nx_native_demo.add(20, 22))


def input_examples():
    section("Switch controller input")
    show("switch_input.available", switch_input.available())
    if switch_input.available():
        switch_input.configure()
        state = switch_input.update()
        show("switch_input state", state)
        show("switch_input.get_gamepad", switch_input.get_gamepad())
        show("switch_input.devices.gamepads", switch_input.devices.gamepads)


def controller_test():
    if not switch_input.available():
        print("switch_input backend is unavailable.")
        _console_update()
        return

    switch_input.configure()
    recent = []
    while _applet_running():
        state = switch_input.update()
        down = state["button_down_names"]
        up = state["button_up_names"]
        if "PLUS" in down:
            _request_exit()
            break
        if "B" in down:
            break
        if down or up:
            if down:
                recent.insert(0, "down: " + ", ".join(down))
            if up:
                recent.insert(0, "up: " + ", ".join(up))
            recent = recent[:14]

        _clear_screen()
        print("Controller input test")
        print("Press buttons. B returns. + exits.")
        print()
        print("Held:", ", ".join(state["button_names"]) or "<none>")
        print("Left stick:", state["left_stick"])
        print("Right stick:", state["right_stick"])
        print("Connected:", state["connected"], "Handheld:", state["handheld"])
        print()
        print("Recent events:")
        for line in recent:
            print(line)
        _console_update()
        _sleep_frame()


def keyboard_input_test():
    if not switch_input.available():
        print("switch_input backend is unavailable.")
        _console_update()
        return

    switch_input.configure()
    _clear_screen()
    print("Python input() keyboard test")
    print("The Switch software keyboard opens next.")
    print()
    print("Submit text to show it here. Cancel returns None/EOF.")
    _console_update()

    try:
        entered = input("Text for input(): ")
    except EOFError:
        entered = None

    prompt_entered = None
    password_entered = None
    while _applet_running():
        state = switch_input.update()
        down = set(state["button_down_names"])
        if "PLUS" in down:
            _request_exit()
            break
        if "B" in down:
            break
        if "A" in down:
            prompt_entered = switch_input.prompt_keyboard(
                "Text for switch_input.prompt_keyboard()",
                keyboard_type=switch_input.KeyboardType.QWERTY,
                initial_text=prompt_entered or "",
            )
        if "X" in down:
            password_entered = switch_input.prompt_keyboard(
                "Password text",
                keyboard_type=switch_input.KeyboardType.QWERTY,
                max_length=64,
                password=True,
            )

        _clear_screen()
        print("Python input() keyboard test")
        print()
        print("input() returned:")
        print(repr(entered))
        print()
        print("switch_input.prompt_keyboard() returned:")
        print(repr(prompt_entered))
        print()
        print("password prompt length:")
        print(None if password_entered is None else len(password_entered))
        print()
        print("A opens switch_input.prompt_keyboard().")
        print("X opens password-style prompt.")
        print("B returns to the menu. + exits.")
        _console_update()
        _sleep_frame()


def flappy_bird():
    if not switch_input.available():
        print("switch_input backend is unavailable.")
        _console_update()
        return

    switch_input.configure()
    width = 38
    height = 18
    bird_x = 7
    bird_y = float(height // 2)
    velocity = 0.0
    gravity = 0.045
    flap = -0.72
    frame = 0
    score = 0
    pipes = [{"x": width - 1, "gap": height // 2}]
    game_over = False

    while _applet_running():
        state = switch_input.update()
        down = set(state["button_down_names"])
        held = set(state["button_names"])

        if "PLUS" in down:
            _request_exit()
            break
        if "B" in down:
            break
        if game_over and "A" in down:
            bird_y = float(height // 2)
            velocity = 0.0
            frame = 0
            score = 0
            pipes = [{"x": width - 1, "gap": height // 2}]
            game_over = False

        if not game_over:
            if down & {"A", "UP", "STICK_L_UP"}:
                velocity = flap
            velocity = min(0.42, velocity + gravity)
            bird_y += velocity

            if frame % 7 == 0:
                for pipe in pipes:
                    pipe["x"] -= 1
                if pipes[-1]["x"] < width - 14:
                    gap = 4 + ((score * 5 + frame // 7) % (height - 8))
                    pipes.append({"x": width - 1, "gap": gap})
                pipes = [pipe for pipe in pipes if pipe["x"] >= 0]

            for pipe in pipes:
                if pipe["x"] == bird_x:
                    if abs(round(bird_y) - pipe["gap"]) > 2:
                        game_over = True
                    else:
                        score += 1
            if bird_y < 0 or bird_y >= height:
                game_over = True
            frame += 1

        cells = [[" " for _ in range(width)] for _ in range(height)]
        for pipe in pipes:
            x = pipe["x"]
            if 0 <= x < width:
                for y in range(height):
                    if abs(y - pipe["gap"]) > 2:
                        cells[y][x] = "|"
        bird_row = round(bird_y)
        if 0 <= bird_row < height:
            cells[bird_row][bird_x] = ">"

        _clear_screen()
        print("PyFlap NX | A/Up flap | B menu | + exit")
        print("Score:", score)
        print("+" + "-" * width + "+")
        for row in cells:
            print("|" + "".join(row) + "|")
        print("+" + "-" * width + "+")
        if game_over:
            print("Game over. Press A to restart.")
        else:
            print("Held:", ", ".join(sorted(held)) or "<none>")
        _console_update()
        _sleep_frame()


FEATURES = [
    ("Filesystem roots", filesystem_examples, False),
    ("Compression", compression_examples, False),
    ("Hashlib", hashlib_examples, False),
    ("Switch backends", backend_examples, False),
    ("Requests adapter setup", requests_groundwork_example, False),
    ("Requests adapter HTTP", requests_http_example, False),
    ("switch_curl direct HTTP", switch_curl_http_example, False),
    ("Native extension", native_extension_example, False),
    ("Controller input snapshot", input_examples, False),
    ("Keyboard input test", keyboard_input_test, True),
    ("Controller input live test", controller_test, True),
    ("PyFlap switch_input game", flappy_bird, True),
]


def _console_update():
    switch = getattr(sys, "switch", None)
    if switch is not None:
        switch.console_update()


def _sleep_frame():
    switch = getattr(sys, "switch", None)
    if switch is not None:
        switch.sleep(16_666_667)


def _applet_running():
    switch = getattr(sys, "switch", None)
    if switch is None:
        return True
    return switch.applet_main_loop()


def _request_exit():
    switch = getattr(sys, "switch", None)
    if switch is not None:
        switch.request_exit()


def _clear_screen():
    print("\x1b[2J\x1b[H", end="")


def _run_feature(feature):
    output = io.StringIO()
    with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
        try:
            feature()
        except BaseException:
            traceback.print_exc()
    return output.getvalue().splitlines()


def _run_all_features():
    lines = []
    for _name, feature, live in FEATURES:
        if live:
            continue
        lines.extend(_run_feature(feature))
    return lines


def _draw_cli(selected, output_lines, offset):
    _clear_screen()
    print("CPython Nintendo Switch feature CLI")
    print(sys.version.split()[0], "| A run | X run all | B menu | + exit")
    print()

    if output_lines is None:
        for index, (name, _feature, live) in enumerate(FEATURES):
            marker = ">" if index == selected else " "
            suffix = " [live]" if live else ""
            print(f"{marker} {name}{suffix}")
        print()
        print("Use D-pad or left stick to choose a feature.")
    else:
        visible = output_lines[offset:offset + 21]
        for line in visible:
            print(line)
        if len(output_lines) > 21:
            print()
            print(f"Showing {offset + 1}-{offset + len(visible)} of {len(output_lines)} | L/R scroll")
        print()
        print("B returns to the menu.")
    _console_update()


def controller_cli():
    if not switch_input.available():
        print("switch_input backend is unavailable; running the feature list once.")
        for _name, feature, live in FEATURES:
            if not live:
                feature()
        return

    switch_input.configure()
    selected = 0
    output_lines = None
    offset = 0
    _draw_cli(selected, output_lines, offset)

    while _applet_running():
        state = switch_input.update()
        buttons = set(state["button_down_names"])

        if "PLUS" in buttons:
            break

        redraw = False
        if output_lines is None:
            if buttons & {"DOWN", "STICK_L_DOWN"}:
                selected = (selected + 1) % len(FEATURES)
                redraw = True
            elif buttons & {"UP", "STICK_L_UP"}:
                selected = (selected - 1) % len(FEATURES)
                redraw = True
            elif "A" in buttons:
                _name, feature, live = FEATURES[selected]
                if live:
                    feature()
                    output_lines = None
                else:
                    output_lines = _run_feature(feature)
                    offset = 0
                redraw = True
            elif "X" in buttons:
                output_lines = _run_all_features()
                offset = 0
                redraw = True
        else:
            if "B" in buttons:
                output_lines = None
                offset = 0
                redraw = True
            elif buttons & {"R", "ZR", "DOWN", "STICK_L_DOWN"}:
                offset = min(max(len(output_lines) - 21, 0), offset + 10)
                redraw = True
            elif buttons & {"L", "ZL", "UP", "STICK_L_UP"}:
                offset = max(0, offset - 10)
                redraw = True

        if redraw:
            _draw_cli(selected, output_lines, offset)
        _sleep_frame()


def main():
    controller_cli()


if __name__ == "__main__":
    main()
