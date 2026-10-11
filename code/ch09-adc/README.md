# 9장 실습: ADC

| 폴더 | 보드 | 내용 |
| --- | --- | --- |
| `avr/` | 아두이노 우노 | A0 가변저항 → 8개 이동 평균 → D11 LED 밝기(Timer2 PWM) |
| `stm32/` | NUCLEO-F411RE | PA0 가변저항 → 이동 평균 → LD2 밝기(TIM2 PWM), 내부 온도 센서와 VDDA 측정 |

STM32 예제의 온도(`temp_c_x100`)와 전원 전압(`vdda_mv`)은 GDB에서 `print`로 확인합니다. 14장에서 UART로 출력하도록 바꿉니다.
