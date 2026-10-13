# 33장 실습: 서명된 이미지와 보안 부트의 핵심

```
ch33-security/
├── get_mbedtls.sh                Mbed TLS v3.6.7을 lib/mbedtls에 받는다
├── common/
│   ├── mbedtls_config_min.h      서명 검증(ECDSA P-256 + SHA-256)만 켠 설정
│   └── secure_image.c/.h         서명된 머리말 형식과 secure_verify()
├── tools/
│   ├── keygen.py                 키 쌍 → signing_key.pem(비밀), pubkey.h(공개)
│   └── sign_image.py             .bin → 서명된 .simg (보안 카운터 포함)
├── test/                         PC 테스트: 변조, 다른 키, 롤백, 잘못된 공개 키
└── stm32/                        보드에서 슬롯 B의 이미지 검증 + RDP 수준 읽기
```

```bash
./get_mbedtls.sh
pip install cryptography
cd test && make                    # 실행할 때마다 새 키를 만든다
cd ../stm32 && make keys && make flash && make signed-test
```

개인 키(`*.pem`, `keys/`)는 `.gitignore`로 막아 두었습니다. 저장소에 올리지 마세요.

빌드 확인: arm-none-eabi-gcc 14.2.1, 경고 0 (Mbed TLS 3.6.7 포함, `-Og`로 text 28,284 B). [확인 필요: 보드에서 검증 시간]
