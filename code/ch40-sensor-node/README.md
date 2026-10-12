# 40장 최종 프로젝트: 보안 OTA를 갖춘 IoT 센서 노드

설계는 [DESIGN.md](DESIGN.md)부터 보세요.

```
ch40-sensor-node/
├── common/
│   ├── link.c/.h          보드 간 UART 프로토콜: COBS + CRC-16 + 정지-대기 ARQ + 중복 거르기 (두 보드 공통)
│   └── ota_writer.c/.h    받은 이미지를 다른 슬롯에 쓰고, 서명 확인 후 시험 부팅 요청
├── test/                  PC 테스트: 잡음 채널(0.1% 손실 + 0.1% 깨짐)에서 300조각 OTA, OTA 쓰기 규칙
├── stm32-boot/            보안 부트로더 (32장 부트로더 + 33장 secure_verify)
├── stm32-node/            센서 노드 앱 (USART1 링크, BMP280, OTA, 자가 진단 후 확정)
└── esp32-bridge/          ESP32 연결부 (Wi-Fi, MQTT/TLS, HTTPS 다운로드, OTA 전달)
```

```bash
cd test && make                                       # PC
(cd ../../ch33-security && ./get_mbedtls.sh)
cd ../stm32-node && make keys && make SLOT=A VER=1.0.0 && make SLOT=A VER=1.0.0 signed
cd ../stm32-boot && make flash
```

[확인 필요: 보안 부트로더 크기와 Flash 배치, 노드·연결부 빌드, 전체 시스템 OTA와 롤백 실측]
