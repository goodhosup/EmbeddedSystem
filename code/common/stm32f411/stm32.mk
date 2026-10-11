# STM32F411 예제 공통 Makefile 규칙
#
# 각 예제의 Makefile에서 TARGET, SRCS를 정한 뒤 이 파일을 include 한다.
#   make          빌드 (build/$(TARGET).elf, .bin)
#   make flash    OpenOCD와 ST-LINK로 업로드
#   make clean    빌드 결과 삭제

COMMON_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CPU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS += $(CPU) -std=c11 -Og -g3 -Wall -Wextra -ffreestanding \
          -ffunction-sections -fdata-sections \
          -fno-tree-loop-distribute-patterns -I$(COMMON_DIR)
LDFLAGS += $(CPU) -nostdlib -T$(COMMON_DIR)stm32f411.ld \
           -Wl,--gc-sections -Wl,-Map=build/$(TARGET).map

SRCS += $(COMMON_DIR)startup.c
OBJS  = $(addprefix build/,$(notdir $(SRCS:.c=.o)))
vpath %.c $(sort $(dir $(SRCS)))

all: build/$(TARGET).bin

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc
	$(SIZE) $@

build/$(TARGET).bin: build/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

build:
	mkdir -p build

flash: build/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program $< verify reset exit"

clean:
	rm -rf build

.PHONY: all flash clean
