# 10장 실습: DMA (STM32 전용)

AVR(ATmega328P, ATmega128)에는 DMA가 없으므로 이 장의 예제는 STM32만 있습니다.

1. **메모리 복사 비교**: 1024워드를 CPU 루프와 DMA로 각각 복사하고 DWT 사이클 카운터로 걸린 클럭 수를 잽니다. GDB에서 `print cpu_copy_cycles`, `print dma_copy_cycles`, `print copy_ok`.
2. **TIM3 → ADC → DMA 원형 버퍼**: TIM3이 1 kHz로 ADC 변환을 시작하고, DMA가 결과를 64칸 원형 버퍼에 쌓습니다. 하프/풀 전송 인터럽트에서 절반씩 평균을 내 LD2 밝기를 정합니다. PA0에 9장의 가변저항을 연결합니다. `print half_count`, `print pot_avg`.
