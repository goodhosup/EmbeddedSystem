# 36장: 공장 시험 펌웨어
TARGET     = ch36-factory
USE_NEWLIB = 1
CH32       = ../../../ch32-bootloader
SRCS       = main.c ../../../ch15-i2c/stm32/i2c_stm32.c ../../../ch35-safety/common/march.c \
             $(CH32)/stm32/flash_f4.c $(CH32)/common/crc32.c ../../../common/stm32f411/uart2.c
CFLAGS    += -I../../../ch15-i2c/common -I../../../ch35-safety/common -I$(CH32)/common

include ../../../common/stm32f411/stm32.mk
