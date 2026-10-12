# 30장 프로젝트: 환경 모니터링 데이터 로거

BMP280 온도를 주기적으로 재서 SD 카드에 CSV 파일로 남깁니다. 설계는 [DESIGN.md](DESIGN.md)를 보세요.

```
ch30-logger/
├── DESIGN.md            설계 문서 (요구 사항 R1~R8, 구조, 시험 계획)
├── get_fatfs.sh         FatFs R0.15a를 lib/fatfs에 받는다
├── ffconf.h             FatFs 설정
├── common/              하드웨어와 무관 (PC에서 시험)
│   ├── sd_spi.c/.h      SD 카드 SPI 모드 드라이버 (16장 spi.h 위)
│   ├── diskio.c         FatFs ↔ sd_spi
│   ├── logrec.c/.h      기록 → CSV 한 줄
│   └── logger.c/.h      파일 만들기, 기록, f_sync
├── stm32/               NUCLEO-F411RE + FreeRTOS
│   ├── main.c           sensor, logger, console 태스크
│   ├── uart_dma.c/.h    USART2 송신 DMA, 수신 인터럽트 → 큐
│   └── FreeRTOSConfig.h 틱 없는 Idle
└── test/                PC 테스트
    ├── fake_sd.c/.h     SPI 바이트 단위로 SD 카드를 흉내 내는 가짜 카드
    └── test_logger.c    CSV, SD 프로토콜, FatFs, 전원 차단, 카드 없음
```

```bash
./get_fatfs.sh
cd test && make                                        # PC 테스트
(cd ../../ch26-freertos/stm32 && ./get_freertos.sh)    # FreeRTOS 커널
cd ../stm32 && make flash                              # 보드
```

명령 (115200 bps): `s` 상태, `f` 지금 저장, `x` 파일 닫기(카드 빼기 전), `r` 새 파일로 다시 시작, `p` 주기 1 s ↔ 10 s

[확인 필요: arm-none-eabi-gcc로 FreeRTOS와 FatFs까지 포함한 전체 빌드의 경고 확인, 보드 통합 시험(30.8절)]
