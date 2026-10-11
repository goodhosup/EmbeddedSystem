# 아두이노 우노(ATmega328P)용 빌드와 업로드
#   make          빌드 (main.hex 생성)
#   make flash    우노의 부트로더로 업로드 (PORT로 포트 지정)
#   make clean    빌드 결과 삭제

MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = main
SRCS    = main.c

PORT       ?= /dev/ttyACM0
PROGRAMMER ?= arduino
BAUD       ?= 115200

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=c11 -Os -Wall -Wextra
LDFLAGS = -mmcu=$(MCU)

all: $(TARGET).hex

$(TARGET).elf: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	avrdude -c $(PROGRAMMER) -p m328p -P $(PORT) -b $(BAUD) -U flash:w:$<:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean
