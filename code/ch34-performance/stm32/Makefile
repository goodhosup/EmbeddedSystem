# 34장: FIR 벤치마크
#   make OPT=-O2 flash      최적화 수준을 바꿔 가며 비교한다 (바꿀 때는 make clean)
TARGET     = ch34-performance
OPT       ?= -O2
USE_NEWLIB = 1
SRCS       = main.c ../common/dsp.c ../../common/stm32f411/uart2.c
CFLAGS    += -I../common -DBUILD_OPT=\"$(OPT)\"

include ../../common/stm32f411/stm32.mk

# 공통 규칙의 -Og 뒤에 붙여서 이 값이 이기게 한다 (GCC는 마지막 -O를 쓴다)
CFLAGS += $(OPT)

# 역어셈블해서 컴파일러가 만든 명령을 본다 (34.4절)
disasm: build/$(TARGET).elf
	arm-none-eabi-objdump -d -S --no-show-raw-insn $< > build/$(TARGET).lst
	@echo "build/$(TARGET).lst"

.PHONY: disasm
