# 3장 실습: 같은 지연 루프, 다른 클럭

같은 코드라도 클럭이 바뀌면 실행 속도가 바뀐다는 것을 LED로 확인합니다.

| 폴더 | 보드 | 내용 |
| --- | --- | --- |
| `avr/` | 아두이노 우노 | 16 MHz → CLKPR로 2 MHz로 낮춤 (느려짐) |
| `stm32/` | NUCLEO-F411RE | HSI 16 MHz → PLL로 100 MHz로 올림 (빨라짐) |

```bash
cd avr && make flash
cd stm32 && make flash      # arm-none-eabi-gcc, OpenOCD 필요 (5장)
```
