TARGET     = ch21-boot
SRCS       = main.c ../../common/stm32f411/uart2.c
USE_NEWLIB = 1

include ../../common/stm32f411/stm32.mk

# 섹션 크기와 기호 보기 (21.7절)
inspect: build/$(TARGET).elf
	arm-none-eabi-size -A $<
	arm-none-eabi-nm -n $< | grep -E ' (_sidata|_sdata|_edata|_sbss|_ebss|_estack|initialized|zeroed|table|main)$$'

.PHONY: inspect
