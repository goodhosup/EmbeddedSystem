# 34장 실습: 측정하고 최적화하기

같은 FIR 필터(256 표본 × 32 탭)를 세 가지로 구현해 비교합니다.

| 함수 | 방식 |
| --- | --- |
| `fir_f32` | float (Cortex-M4F FPU) |
| `fir_q15_ref` | Q15, 기준 구현 (64비트 누산) |
| `fir_q15_fast` | Q15, SIMD `SMLALD`로 두 탭씩 (`__ARM_FEATURE_DSP`일 때), 결과는 기준과 비트 단위로 같음 |

```bash
cd test && make                          # PC: Q15 연산, 빠른 구현 = 기준 구현, float과의 오차
cd ../stm32
make clean && make OPT=-O2 flash         # 16 MHz / 100 MHz ART 켬 / 100 MHz ART 끔
make disasm                              # 역어셈블 → build/ch34-performance.lst
```

[확인 필요: 보드 실측값 (34.7절 표)]
