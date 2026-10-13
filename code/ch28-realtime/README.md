# 28장 실습: 실시간성 분석

## host/: 스케줄 가능성 분석 도구 (PC)

```bash
cd host
make test      # 알려진 값으로 자체 시험
./rta          # 예제 태스크 집합(ctrl, comm, log)의 이용률과 응답 시간
```

`rta_main.c`의 태스크 목록을 바꿔 자기 시스템을 분석할 수 있습니다.

## stm32/: 지연·응답 시간 측정과 간이 트레이스 (NUCLEO-F411RE, FreeRTOS)

FreeRTOS 커널은 26장, UART 드라이버는 27장 예제의 것을 함께 씁니다.

```bash
(cd ../ch26-freertos/stm32 && ./get_freertos.sh)
cd stm32
make flash     # 터미널 115200 bps
```

| 명령 | 보는 것 |
| --- | --- |
| `l` | TIM2 인터럽트 지연의 최소, 최대, 분포 |
| `c` | 크리티컬 섹션(100 µs) 태스크 켜기/끄기 |
| `p` | TIM2 우선순위 6 ↔ 2 |
| `r` | 태스크별 최대 응답 시간과 RTA 예측 |
| `t` | 최근 문맥 전환 32개 |

빌드 확인: arm-none-eabi-gcc 14.2.1, 경고 0 (FreeRTOS V11.1.0 포함, text 18,648 B). [확인 필요: 보드 실측값]
