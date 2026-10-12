# 24장 실습: 이벤트 기반 세탁기

상태 머신(`app/`), 이벤트 큐와 협력형 스케줄러(`service/`), 보드 계층(`stm32/`)으로 나눈 예제입니다.
`app/`과 `service/`는 하드웨어 헤더를 포함하지 않으므로 PC에서 그대로 시험할 수 있습니다.

```bash
cd test && make          # PC 테스트
cd stm32 && make flash   # NUCLEO-F411RE
```

| 입력 | 동작 |
| --- | --- |
| B1 또는 `s` | 시작 / 일시정지 / 재개 |
| `o` | 문 열기 (DONE → IDLE) |
| `t` | 태스크별 최대 실행 시간 |

LD2: IDLE 꺼짐, 동작 중 빠른 깜박임, PAUSED 느린 깜박임, DONE 켜짐.
