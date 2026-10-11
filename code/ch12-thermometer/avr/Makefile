# 12장 프로젝트: 디지털 온도계 (아두이노 우노)
#   make VREF_MV=1085     내부 기준 전압을 멀티미터로 잰 값으로 보정
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = thermometer
SRCS    = main.c tick.c button.c sensor.c uart.c

PORT   ?= /dev/ttyACM0

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu11 -Os -Wall -Wextra
ifdef VREF_MV
CFLAGS += -DVREF_MV=$(VREF_MV)u
endif

all: $(TARGET).hex

$(TARGET).elf: $(SRCS) $(wildcard *.h)
	avr-gcc $(CFLAGS) -o $@ $(SRCS)
	avr-size $@

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	avrdude -c arduino -p m328p -P $(PORT) -b 115200 -U flash:w:$<:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean
