#!/usr/bin/env python3
"""33장: .bin에 서명된 머리말을 붙인다 (32장 mkimage.py + ECDSA P-256 서명).

    python3 sign_image.py app.bin --key signing_key.pem --load-addr 0x08040000 \\
            --version 1.2.0 --counter 3 -o app.simg
"""
import argparse
import hashlib
import struct
import sys

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

MAGIC = 0x31474953
HDR_SIZE = 0x200
SIGNED_HDR = 0x40


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bin")
    ap.add_argument("--key", required=True)
    ap.add_argument("--load-addr", type=lambda s: int(s, 0), required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--counter", type=int, required=True, help="보안 카운터 (롤백 방지)")
    ap.add_argument("-o", "--output", required=True)
    a = ap.parse_args()

    body = open(a.bin, "rb").read()
    body += b"\xff" * (-len(body) % 4)
    ma, mi, pa = (int(x) for x in a.version.split("."))
    head = struct.pack("<6I", MAGIC, HDR_SIZE, a.load_addr, len(body), (ma << 16) | (mi << 8) | pa, a.counter)
    head += b"\x00" * (SIGNED_HDR - len(head))

    key = serialization.load_pem_private_key(open(a.key, "rb").read(), password=None)
    digest = hashlib.sha256(head + body).digest()
    der = key.sign(digest, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
    r, s = utils.decode_dss_signature(der)
    sig = r.to_bytes(32, "big") + s.to_bytes(32, "big")

    out = head + sig
    out += b"\xff" * (HDR_SIZE - len(out))
    open(a.output, "wb").write(out + body)
    print(f"{a.output}: v{a.version}, counter {a.counter}, {len(body)} bytes, sha256 {digest.hex()[:16]}...")
    return 0


if __name__ == "__main__":
    sys.exit(main())
