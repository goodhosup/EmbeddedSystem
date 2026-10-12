# 18장 USB CDC (TinyUSB)
#   ./get_tinyusb.sh   먼저 한 번 실행해서 TinyUSB를 lib/tinyusb에 받는다
#   make
TARGET     = ch18-usb
TINYUSB   ?= lib/tinyusb
USE_NEWLIB = 1

SRCS  = main.c clock.c usb_descriptors.c
SRCS += $(TINYUSB)/src/tusb.c \
        $(TINYUSB)/src/common/tusb_fifo.c \
        $(TINYUSB)/src/device/usbd.c \
        $(TINYUSB)/src/device/usbd_control.c \
        $(TINYUSB)/src/class/cdc/cdc_device.c \
        $(TINYUSB)/src/portable/synopsys/dwc2/dcd_dwc2.c \
        $(wildcard $(TINYUSB)/src/portable/synopsys/dwc2/dwc2_common.c)

CFLAGS += -I. -I$(TINYUSB)/src \
          -I$(TINYUSB)/hw/mcu/st/cmsis_device_f4/Include \
          -I$(TINYUSB)/lib/CMSIS_5/CMSIS/Core/Include \
          -DSTM32F411xE -DCFG_TUSB_MCU=OPT_MCU_STM32F4

include ../../common/stm32f411/stm32.mk
