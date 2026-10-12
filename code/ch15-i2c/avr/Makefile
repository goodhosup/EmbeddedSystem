# 15장 I2C (아두이노 우노)
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = i2c
UART    = ../../ch14-uart/avr
SRCS    = main.c i2c_avr.c ../common/bmp280.c ../common/app.c $(UART)/uart.c

PORT   ?= /dev/ttyACM0

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu11 -Os -Wall -Wextra -I../common -I$(UART)

all: $(TARGET).hex

$(TARGET).elf: $(SRCS)
	avr-gcc $(CFLAGS) -o $@ $(SRCS)
	avr-size $@

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	avrdude -c arduino -p m328p -P $(PORT) -b 115200 -U flash:w:$<:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean
