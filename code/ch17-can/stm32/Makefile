#   make NODE_ID=0x200 MCP_OSC_HZ=8000000
TARGET     = ch17-can
SPI        = ../../ch16-spi
SRCS       = main.c $(SPI)/stm32/spi_stm32.c ../common/mcp2515.c ../common/app.c ../../common/stm32f411/uart2.c
NODE_ID    ?= 0x200
MCP_OSC_HZ ?= 8000000
CFLAGS    += -I../common -I$(SPI)/common -DNODE_ID=$(NODE_ID)u -DMCP_OSC_HZ=$(MCP_OSC_HZ)u
USE_NEWLIB = 1

include ../../common/stm32f411/stm32.mk
