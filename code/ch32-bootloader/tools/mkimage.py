#!/usr/bin/env python3
"""32장: 빌드된 앱(.bin) 앞에 이미지 머리말(512 B)을 붙인다. MCUboot의 imgtool이 하는 일의 축소판.

    python3 mkimage.py build/app-B.bin --slot B --version 1.1.0 -o build/app-B.img

머리말 형식은 common/image.h와 같다 (리틀 엔디언 32비트 8개):
    magic 'IMG1', hdr_size 0x200, load_addr(슬롯 시작), img_size, version, crc32, flags, hdr_crc
"""
import argparse
import struct
import sys
import zlib

SLOTS = {"A": 0x08020000, "B": 0x08040000}
SLOT_SIZE = 128 * 1024
HDR_SIZE = 0x200
MAGIC = 0x31474D49


def parse_version(s: str) -> int:
    major, minor, patch = (int(x) for x in s.split("."))
    if not (0 <= major <= 0xFFFF and 0 <= minor <= 0xFF and 0 <= patch <= 0xFF):
        raise ValueError("version out of range")
    return (major << 16) | (minor << 8) | patch


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bin")
    ap.add_argument("--slot", choices=SLOTS.keys(), required=True)
    ap.add_argument("--version", required=True, help="major.minor.patch")
    ap.add_argument("-o", "--output", required=True)
    a = ap.parse_args()

    body = open(a.bin, "rb").read()
    body += b"\xff" * (-len(body) % 4)              # 4바이트 단위로 (Flash 쓰기 단위)
    base = SLOTS[a.slot]
    if len(body) > SLOT_SIZE - HDR_SIZE:
        print(f"image too big: {len(body)} bytes", file=sys.stderr)
        return 1

    # 링크 주소 확인: 리셋 벡터가 이 슬롯 안을 가리켜야 한다 (SLOT을 잘못 주고 빌드한 실수를 잡는다)
    sp, reset = struct.unpack_from("<II", body, 0)
    if not (base + HDR_SIZE <= (reset & ~1) < base + SLOT_SIZE) or not (reset & 1):
        print(f"reset vector 0x{reset:08X} is not in slot {a.slot}: linked for another slot?", file=sys.stderr)
        return 1

    fields = [MAGIC, HDR_SIZE, base, len(body), parse_version(a.version), zlib.crc32(body), 0]
    head = struct.pack("<7I", *fields)
    head += struct.pack("<I", zlib.crc32(head))     # hdr_crc: 앞 28바이트
    head += b"\xff" * (HDR_SIZE - len(head))
    with open(a.output, "wb") as f:
        f.write(head + body)
    print(f"{a.output}: slot {a.slot} @0x{base:08X}, v{a.version}, {len(body)} bytes, crc32 0x{zlib.crc32(body):08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
