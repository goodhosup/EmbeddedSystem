# 32장: 부트로더 (0x0800 0000, 32 KB)
#   make flash 로 올린다 (섹터 0~1만 지운다)
TARGET   = ch32-boot
LDSCRIPT = boot.ld
SRCS     = main.c ../flash_f4.c ../../common/crc32.c ../../common/image.c \
           ../../common/bootmeta.c ../../common/bootlogic.c ../../../common/stm32f411/uart2.c
CFLAGS  += -I../../common
LDFLAGS += -L..

include ../../../common/stm32f411/stm32.mk
