# 아두이노 우노
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = lowpower

PORT   ?= /dev/ttyACM0

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu11 -Os -Wall -Wextra

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
