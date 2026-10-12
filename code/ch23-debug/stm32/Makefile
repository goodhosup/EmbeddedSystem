TARGET     = ch23-debug
SRCS       = main.c fault.c ../../common/stm32f411/uart2.c
USE_NEWLIB = 1

include ../../common/stm32f411/stm32.mk

# 폴트 출력의 PC를 소스 줄로: make line PC=0x080004a2
line:
	arm-none-eabi-addr2line -f -e build/$(TARGET).elf $(PC)

.PHONY: line
