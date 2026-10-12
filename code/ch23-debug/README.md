# 23장 실습: 폴트 일으키고 원인 읽기

`stm32/`의 예제는 시리얼 명령으로 일부러 폴트를 일으키고, HardFault 처리기가 스택 프레임의 PC와 CFSR 등을 출력합니다.

```bash
cd stm32
make flash
# 터미널(115200)에서 d, b, t, s를 눌러 본다
make line PC=0x08000204      # 출력된 PC를 소스 줄로 (gcc 13.2.1 기준 d 명령의 PC)
```

| 명령 | 일으키는 폴트 | 기대하는 CFSR 비트 |
| --- | --- | --- |
| `d` | 0으로 나누기 | DIVBYZERO (HardFault로 FORCED) |
| `b` | 없는 주소(0x6000 0000) 읽기 | PRECISERR, BFARVALID |
| `t` | Thumb 비트 없는 주소로 점프 | INVSTATE (PC는 점프 목적지이므로 LR로 호출한 곳을 찾는다) |
| `s` | 깊은 재귀로 스택 넘침 | 결과 관찰 (25장) |
