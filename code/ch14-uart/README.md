# 14장 실습: UART 명령어 셸

| 폴더 | 보드 | 내용 |
| --- | --- | --- |
| `avr/` | 아두이노 우노 | USART0, 수신·송신 모두 인터럽트 + 링 버퍼, avr-libc stdio로 printf |
| `stm32/` | NUCLEO-F411RE | USART2(PA2/PA3, ST-LINK 가상 COM), 수신 인터럽트 + 링 버퍼, newlib-nano printf |

시리얼 터미널을 **115200 bps, 8N1**로 열고 `help`를 입력합니다. 아두이노 IDE의 시리얼 모니터를 쓴다면 줄 끝 설정을 "Newline"으로 둡니다.

```
ch14 UART shell. type help
> help
help, led on|off, adc, uptime, echo <text>
> led on
LED on
> adc
A0 = 512 (2500 mV)
```

STM32 예제는 printf를 쓰기 위해 newlib-nano를 링크합니다(`USE_NEWLIB = 1`). 공통 파일 `code/common/stm32f411/syscalls.c`가 `_write()`를 `__io_putchar()`로 연결합니다.
