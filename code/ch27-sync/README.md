# 27장 실습: 태스크 간 동기화와 통신

FreeRTOS 커널은 26장 예제가 받아 둔 것을 함께 씁니다.

```bash
(cd ../ch26-freertos/stm32 && ./get_freertos.sh)
cd stm32
make flash               # 터미널 115200 bps
```

| 입력 | 보는 것 |
| --- | --- |
| `r` | 경쟁 조건: 보호 없이 / 뮤텍스로 공유 카운터 증가 |
| `i` | 우선순위 역전: 이진 세마포어 / 뮤텍스(우선순위 상속)로 high의 대기 시간 |
| `s` | 생산자–소비자 큐 상태 |
| B1 | EXTI ISR → 태스크 알림 → 디바운싱 → 출력 (지연 처리) |

UART 수신은 `uart_rtos.c`의 ISR이 FreeRTOS 큐에 넣습니다(공통 `uart2.c` 대신).

빌드 확인: arm-none-eabi-gcc 14.2.1, 경고 0 (FreeRTOS V11.1.0 포함, text 20,324 B). [확인 필요: r·i 명령의 보드 실측값]
