# 32장: 슬롯 A/B용 앱
#   make SLOT=A VER=1.0.0 image      → build/app-A.img (머리말 + 앱)
#   make SLOT=A flash-img            → 0x0802 0000에 올림 (처음 한 번)
#   make SLOT=B VER=1.1.0 image      → build/app-B.img: send_update.py로 보낼 새 버전
# 같은 소스를 슬롯마다 다른 주소로 링크한다. 슬롯이나 버전을 바꿀 때는 make clean 먼저.
SLOT     ?= A
VER      ?= 1.0.0
TARGET    = app-$(SLOT)
LDSCRIPT  = app_slot_$(SLOT).ld
SRCS      = main.c ../flash_f4.c ../../common/crc32.c ../../common/image.c \
            ../../common/bootmeta.c ../../common/bootlogic.c ../../../common/stm32f411/uart2.c
CFLAGS   += -I../../common -DAPP_VERSION_STR=\"$(VER)\"
LDFLAGS  += -L..
SLOT_ADDR = $(if $(filter B,$(SLOT)),0x08040000,0x08020000)

include ../../../common/stm32f411/stm32.mk

image: build/$(TARGET).bin
	python3 ../../tools/mkimage.py $< --slot $(SLOT) --version $(VER) -o build/$(TARGET).img

flash-img: image
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program build/$(TARGET).img $(SLOT_ADDR) verify reset exit"

.PHONY: image flash-img
