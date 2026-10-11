# 아두이노 우노. make SPEED=1 이면 최고 속도 토글 시험 버전으로 빌드한다.
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = gpio

PORT       ?= /dev/ttyACM0

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=c11 -Os -Wall -Wextra
ifdef SPEED
CFLAGS += -DSPEED_TEST
endif

all: $(TARGET).hex

$(TARGET).elf: main.c
	avr-gcc $(CFLAGS) -o $@ $<
	avr-size $@

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@

disasm: $(TARGET).elf
	avr-objdump -d -S $< > $(TARGET).lst

flash: $(TARGET).hex
	avrdude -c arduino -p m328p -P $(PORT) -b 115200 -U flash:w:$<:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).lst

.PHONY: all disasm flash clean
