#!/usr/bin/env python3
"""33장: 서명용 키 쌍(ECDSA P-256)을 만든다.

    python3 keygen.py OUTDIR

OUTDIR/signing_key.pem   개인 키: 절대 저장소에 올리지 않는다. 실제 제품은 HSM이나 키 관리 서비스에 둔다.
OUTDIR/pubkey.h          공개 키를 C 배열로: 부트로더(검증 쪽) 코드에 넣는다.
pip install cryptography
"""
import sys
from pathlib import Path

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    out.mkdir(parents=True, exist_ok=True)
    key = ec.generate_private_key(ec.SECP256R1())
    (out / "signing_key.pem").write_bytes(key.private_bytes(
        serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
    pub = key.public_key().public_bytes(serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint)
    body = ",\n    ".join(", ".join(f"0x{b:02X}" for b in pub[i:i + 13]) for i in range(0, len(pub), 13))
    (out / "pubkey.h").write_text(
        "/* tools/keygen.py가 만든 공개 키 (ECDSA P-256, 비압축 65 B). 이 파일은 공개해도 된다 */\n"
        "#include <stdint.h>\n"
        f"static const uint8_t signing_pubkey[65] = {{\n    {body}\n}};\n")
    print(f"wrote {out / 'signing_key.pem'} (keep secret) and {out / 'pubkey.h'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
