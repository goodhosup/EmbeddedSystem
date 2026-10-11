# AVR Blink: 우노(기본)와 ATmega128(BOARD=m128)
#
#   make                  빌드
#   make flash            업로드
#   make fuses            퓨즈 읽기 (USBasp, ATmega128 전용)
#   make size             메모리 사용량 보기
#   make disasm           역어셈블 결과(main.lst) 만들기

BOARD ?= uno

ifeq ($(BOARD),m128)
MCU        = atmega128
AVRDUDE_P  = m128
PROGRAMMER = usbasp
PORT       = usb
# 칩 클럭이 낮을 때(공장 출하 1 MHz) ISP 속도를 낮춘다
BITCLOCK  ?= 10
EXTRA      = -B $(BITCLOCK)
else
MCU        = atmega328p
AVRDUDE_P  = m328p
PROGRAMMER = arduino
PORT      ?= /dev/ttyACM0
EXTRA      = -b 115200
endif

F_CPU  ?= 16000000UL
TARGET  = blink

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=c11 -Os -Wall -Wextra

all: $(TARGET).hex

$(TARGET).elf: main.c
	avr-gcc $(CFLAGS) -o $@ $<

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@

size: $(TARGET).elf
	avr-size --format=avr --mcu=$(MCU) $<

disasm: $(TARGET).elf
	avr-objdump -d -S $< > $(TARGET).lst

flash: $(TARGET).hex
	avrdude -c $(PROGRAMMER) -p $(AVRDUDE_P) -P $(PORT) $(EXTRA) -U flash:w:$<:i

fuses:
	avrdude -c usbasp -p m128 -B 10 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).lst

.PHONY: all size disasm flash fuses clean
