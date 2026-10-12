# 15장 실습: I2C 버스 스캔과 BMP280/BME280 온도 센서

| 폴더 | 내용 |
| --- | --- |
| `common/i2c.h` | 보드와 상관없는 I2C 마스터 인터페이스 |
| `common/bmp280.c` | BMP280/BME280 온도 드라이버 (i2c.h만 사용, 두 보드에서 그대로 빌드) |
| `common/app.c` | 버스 스캔, 센서 초기화, 1초마다 온도 출력 |
| `avr/i2c_avr.c` | ATmega328P TWI 구현 (A4 = SDA, A5 = SCL) |
| `stm32/i2c_stm32.c` | STM32F411 I2C1 구현 (PB9 = SDA, PB8 = SCL, AF4) |

```
I2C scan: 0x76
sensor at 0x76, chip id 0x60 (BME280)
T = 24.31 C
```

센서 모듈의 전원 전압(3.3 V 또는 5 V)과 풀업 저항, 레벨 변환 유무를 모듈 설명서에서 확인하세요(15.7절).
AVR 예제는 14장의 `uart.c`를 함께 빌드합니다.

`test/`에서 `make test`를 실행하면 센서 드라이버를 하드웨어 없이 PC에서 시험합니다(데이터시트의 계산 예와 비교).
