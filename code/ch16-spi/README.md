# 16장 실습: SPI 플래시 (W25Q 계열)

| 폴더 | 내용 |
| --- | --- |
| `common/spi.h` | 보드와 상관없는 SPI 마스터 인터페이스 (모드 0, MSB 먼저) |
| `common/w25q.c` | W25Q 계열 SPI NOR 플래시 드라이버 (JEDEC ID, 읽기, 섹터 지우기, 페이지 단위 쓰기) |
| `common/app.c` | ID 읽기 → 섹터 지우기 → 페이지 경계를 넘는 300바이트 쓰기 → 읽어서 검증 |
| `avr/spi_avr.c` | ATmega328P SPI (D13 SCK, D12 MISO, D11 MOSI, D10 CS) |
| `stm32/spi_stm32.c` | STM32F411 SPI1 (PA5 SCK, PA6 MISO, PA7 MOSI: AF5, PB6 CS) |
| `test/` | 가짜 플래시로 드라이버를 PC에서 시험 (`make test`) |

```
JEDEC ID: EF 40 17
sector erase: ok (47 ms)
write 300 bytes: ok (2 ms)
verify: ok (0 mismatches)
```

W25Q 플래시는 3.3 V 칩입니다. NUCLEO에는 바로 연결하고, 5 V 우노에는 레벨 변환 회로를 거쳐 연결하세요(16.6절).
