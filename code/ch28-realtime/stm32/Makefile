# 28장 실시간성 분석 (FreeRTOS)
#   FreeRTOS 커널은 26장 예제가 받아 둔 것을, UART 드라이버는 27장 것을 함께 쓴다:
#     (cd ../../ch26-freertos/stm32 && ./get_freertos.sh)
#   make
TARGET     = ch28-realtime
FREERTOS  ?= ../../ch26-freertos/stm32/lib/FreeRTOS-Kernel
USE_NEWLIB = 1

SRCS  = main.c ../../ch27-sync/stm32/uart_rtos.c
SRCS += $(FREERTOS)/tasks.c \
        $(FREERTOS)/queue.c \
        $(FREERTOS)/list.c \
        $(FREERTOS)/timers.c \
        $(FREERTOS)/event_groups.c \
        $(FREERTOS)/portable/GCC/ARM_CM4F/port.c \
        $(FREERTOS)/portable/MemMang/heap_4.c

CFLAGS += -I. -I../../ch27-sync/stm32 -I$(FREERTOS)/include -I$(FREERTOS)/portable/GCC/ARM_CM4F

include ../../common/stm32f411/stm32.mk
