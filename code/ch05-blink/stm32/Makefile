TARGET = ch05-blink
SRCS   = main.c

include ../../common/stm32f411/stm32.mk

# OpenOCD를 GDB 서버로 실행한다. 다른 터미널에서:
#   arm-none-eabi-gdb build/ch05-blink.elf    (Debian/Ubuntu는 gdb-multiarch)
#   (gdb) target extended-remote :3333
debug: build/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

.PHONY: debug
