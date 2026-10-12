#!/usr/bin/env python3
"""32장: 앱의 'u' 명령으로 새 이미지를 UART로 보낸다.

    python3 send_update.py --port /dev/ttyACM0 build/app-B.img

규약은 app/main.c의 receive_update() 주석과 같다. 묶음마다 '#' 응답을 기다리므로
앱이 Flash에 쓰는 동안 데이터를 잃지 않는다(흐름 제어). pyserial이 필요하다.
"""
import argparse
import struct
import sys
import time

import serial

CHUNK = 256


def wait_for(port, token: bytes, timeout: float) -> bytes:
    buf = b""
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        buf += port.read(64)
        if token in buf:
            return buf
        if b"ERROR" in buf:
            break
    sys.stdout.write(buf.decode(errors="replace"))
    raise RuntimeError(f"did not get {token!r}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("image")
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    a = ap.parse_args()

    data = open(a.image, "rb").read()
    magic, _, load_addr = struct.unpack_from("<III", data, 0)
    if magic != 0x31474D49:
        print("not an image made by mkimage.py", file=sys.stderr)
        return 1
    slot = "A" if load_addr == 0x08020000 else "B"

    with serial.Serial(a.port, a.baud, timeout=0.2) as port:
        port.reset_input_buffer()
        port.write(b"s")
        status = wait_for(port, b"slot B:", 2.0).decode(errors="replace")
        if f"running from slot {slot}" in status:
            print(f"the board is running from slot {slot}; build the image for the other slot", file=sys.stderr)
            return 1

        port.write(b"u")
        wait_for(port, b"READY", 2.0)
        port.write(struct.pack("<I", len(data)))
        wait_for(port, b"ERASED", 10.0)               # 128 KB 섹터 지우기: 1~2초
        for off in range(0, len(data), CHUNK):
            port.write(data[off:off + CHUNK])
            wait_for(port, b"#", 3.0)
            print(f"\r{off + min(CHUNK, len(data) - off)}/{len(data)} bytes", end="", flush=True)
        print()
        print(wait_for(port, b"\n", 10.0).decode(errors="replace").strip())
    return 0


if __name__ == "__main__":
    sys.exit(main())
