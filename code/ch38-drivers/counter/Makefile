# 38장: 커널 모듈 빌드 (지금 실행 중인 커널의 헤더로)
#   sudo apt install linux-headers-$(uname -r)   (Raspberry Pi OS는 raspberrypi-kernel-headers일 수 있다)
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
