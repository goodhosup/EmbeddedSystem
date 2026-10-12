# 30장 프로젝트: 데이터 로거
#   (cd ../../ch26-freertos/stm32 && ./get_freertos.sh)   FreeRTOS 커널
#   (cd .. && ./get_fatfs.sh)                             FatFs
#   make
TARGET     = ch30-logger
FREERTOS  ?= ../../ch26-freertos/stm32/lib/FreeRTOS-Kernel
FATFS     ?= ../lib/fatfs/source
USE_NEWLIB = 1

SRCS  = main.c uart_dma.c
SRCS += ../common/sd_spi.c ../common/diskio.c ../common/logrec.c ../common/logger.c
SRCS += ../../ch15-i2c/stm32/i2c_stm32.c ../../ch15-i2c/common/bmp280.c
SRCS += ../../ch16-spi/stm32/spi_stm32.c
SRCS += $(FATFS)/ff.c
SRCS += $(FREERTOS)/tasks.c \
        $(FREERTOS)/queue.c \
        $(FREERTOS)/list.c \
        $(FREERTOS)/timers.c \
        $(FREERTOS)/portable/GCC/ARM_CM4F/port.c \
        $(FREERTOS)/portable/MemMang/heap_4.c

CFLAGS += -I. -I.. -I../common -I../../ch15-i2c/common -I../../ch16-spi/common \
          -I$(FATFS) -I$(FREERTOS)/include -I$(FREERTOS)/portable/GCC/ARM_CM4F

include ../../common/stm32f411/stm32.mk
