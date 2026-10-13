# 26장 실습: FreeRTOS 태스크와 선점

```bash
cd stm32
./get_freertos.sh        # FreeRTOS 커널 V11.1.0을 lib/에 받는다 (한 번만)
make flash               # 터미널 115200 bps
```

| 명령 | 하는 일 |
| --- | --- |
| `l` | 태스크 목록: 상태, 우선순위, 남은 스택, 힙 |
| `h` | 우선순위 1의 hog 태스크(바쁜 계산) 켜기/끄기 |
| `j` | 우선순위 3인 sample 태스크의 10 ms 주기 지터 |
| `o` | 스택이 작은 태스크를 만들어 스택 오버플로 훅을 확인 |

hog를 켜면 Idle 훅 카운터가 멈추고 blink는 hog와 번갈아 실행되지만, 더 높은 우선순위의 sample과 console은 영향을 거의 받지 않습니다.

빌드 확인: arm-none-eabi-gcc 14.2.1, 경고 0 (FreeRTOS V11.1.0 포함, text 17,028 B, bss 25,704 B). [확인 필요: 보드 실행 결과]
