# 26장 FreeRTOS
#   ./get_freertos.sh   먼저 한 번 실행해서 FreeRTOS 커널을 lib/에 받는다
#   make
TARGET     = ch26-freertos
FREERTOS  ?= lib/FreeRTOS-Kernel
USE_NEWLIB = 1

SRCS  = main.c ../../common/stm32f411/uart2.c
SRCS += $(FREERTOS)/tasks.c \
        $(FREERTOS)/queue.c \
        $(FREERTOS)/list.c \
        $(FREERTOS)/timers.c \
        $(FREERTOS)/portable/GCC/ARM_CM4F/port.c \
        $(FREERTOS)/portable/MemMang/heap_4.c

CFLAGS += -I. -I$(FREERTOS)/include -I$(FREERTOS)/portable/GCC/ARM_CM4F

include ../../common/stm32f411/stm32.mk
