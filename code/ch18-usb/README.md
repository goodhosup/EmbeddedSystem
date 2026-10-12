# 18장 실습: STM32를 USB 가상 시리얼 장치로 (TinyUSB)

STM32F411의 USB OTG FS로 CDC ACM(가상 시리얼) 장치를 만듭니다. USB 스택은 오픈소스 [TinyUSB](https://github.com/hathach/tinyusb)(MIT 라이선스)를 씁니다.

```bash
cd stm32
./get_tinyusb.sh        # TinyUSB와 STM32F4 CMSIS 헤더를 lib/ 아래로 받는다 (한 번만)
make
make flash
```

## 연결

NUCLEO 보드의 USB 커넥터는 ST-LINK용입니다. STM32 자신의 USB는 따로 연결해야 합니다.

| USB 케이블 | NUCLEO |
| --- | --- |
| D+ (초록) | PA12 (CN10 12번 핀) |
| D− (흰색) | PA11 (CN10 14번 핀) |
| GND (검정) | GND |
| VBUS (빨강) | 연결하지 않음 (보드는 ST-LINK USB로 전원) |

[확인 필요: CN10 핀 번호]

PC에서 새 시리얼 포트(리눅스 `/dev/ttyACM1` 등)가 생기면, 터미널로 열고 글자를 입력해 보세요. 대문자로 돌아옵니다. 리눅스에서는 `lsusb -v -d 1209:0001`로 디스크립터를 볼 수 있습니다.

AVR(ATmega328P, ATmega128)에는 USB 주변장치가 없어서 이 장의 AVR 예제는 없습니다(18.8절).
