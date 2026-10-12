# 37장 실습: Raspberry Pi 사용자 공간에서 GPIO와 I2C

| 파일 | 내용 |
| --- | --- |
| `gpio_button_led.c` | GPIO 문자 장치(uAPI v2)로 버튼(GPIO27, 풀업, 하강 에지 이벤트)과 LED(GPIO17) |
| `i2c_linux.c` | 15장 `i2c.h`를 `/dev/i2c-1`의 `ioctl(I2C_RDWR)`로 구현 |
| `bmp280_read.c` | 15장 `bmp280.c`를 고치지 않고 Raspberry Pi에서 사용 |

```bash
make                                 # Raspberry Pi에서
make CROSS=aarch64-linux-gnu-        # PC에서 교차 빌드
./gpio_button_led
./bmp280_read
```

Raspberry Pi 5(Raspberry Pi OS, 커널 6.18)에서 경고 없이 빌드됨을 확인했습니다.

[확인 필요: 버튼·LED·BMP280을 연결한 실행 결과]
