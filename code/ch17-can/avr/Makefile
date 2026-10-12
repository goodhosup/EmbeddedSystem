# 17장 CAN (아두이노 우노)
#   make NODE_ID=0x100 MCP_OSC_HZ=8000000
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = can
UART    = ../../ch14-uart/avr
SPI     = ../../ch16-spi
SRCS    = main.c $(SPI)/avr/spi_avr.c ../common/mcp2515.c ../common/app.c $(UART)/uart.c

PORT       ?= /dev/ttyACM0
NODE_ID    ?= 0x100
MCP_OSC_HZ ?= 8000000

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu11 -Os -Wall -Wextra \
          -I../common -I$(SPI)/common -I$(UART) -DNODE_ID=$(NODE_ID)u -DMCP_OSC_HZ=$(MCP_OSC_HZ)u

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
