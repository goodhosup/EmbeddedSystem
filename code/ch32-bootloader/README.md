# 32장 실습: 부트로더와 펌웨어 업데이트

```
ch32-bootloader/
├── common/              하드웨어와 무관 (부트로더와 앱이 함께 쓰고, PC에서 시험)
│   ├── layout.h         Flash 배치 (부트로더 32 KB, 부트 상태 2섹터, 슬롯 A/B 128 KB)
│   ├── flash_if.h       Flash 지우기/쓰기 인터페이스
│   ├── crc32.c          CRC-32 (zlib과 같은 값)
│   ├── image.c/.h       이미지 머리말 검사
│   ├── bootmeta.c/.h    부트 상태 기록 (덧붙이기, 두 섹터 번갈아)
│   └── bootlogic.c/.h   시험 부팅, 확정, 롤백 규칙
├── test/                PC 테스트 (NOR Flash 흉내, 전원 차단 주입)
├── tools/
│   ├── mkimage.py       .bin → 머리말 붙인 .img
│   └── send_update.py   UART로 이미지 보내기 (pyserial)
├── stm32/
│   ├── flash_f4.c       STM32F411 Flash 드라이버
│   ├── sections.ld      21장 링커 스크립트의 SECTIONS
│   ├── boot/            부트로더 (0x0800 0000)
│   └── app/             슬롯 A/B용 앱 (make SLOT=A|B VER=x.y.z image)
└── esp32-ota/           ESP-IDF OTA와 롤백 (20장의 wifi.c를 씀)
```

```bash
cd test && make                                   # PC 테스트
cd ../stm32/boot && make flash                    # 부트로더
cd ../app && make clean && make SLOT=A VER=1.0.0 flash-img
make clean && make SLOT=B VER=1.1.0 image
python3 ../../tools/send_update.py --port /dev/ttyACM0 build/app-B.img
```

앱 명령: `s` 상태, `c` 확정, `u` 받기, `x` 멈춘 척(워치독 리셋), `r` 리셋

[확인 필요: arm-none-eabi-gcc 빌드 경고, 보드에서 업데이트·롤백 실측, ESP-IDF 5.x 빌드와 OTA 실행]
