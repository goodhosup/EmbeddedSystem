# 아두이노 우노(ATmega328P)용. 사용법은 code/ch02-button-led/Makefile과 같다.
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = main
SRCS    = main.c

PORT       ?= /dev/ttyACM0
PROGRAMMER ?= arduino
BAUD       ?= 115200

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=c11 -Os -Wall -Wextra

all: $(TARGET).hex

$(TARGET).elf: $(SRCS)
	avr-gcc $(CFLAGS) -o $@ $^
	avr-size $@

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	avrdude -c $(PROGRAMMER) -p m328p -P $(PORT) -b $(BAUD) -U flash:w:$<:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean
