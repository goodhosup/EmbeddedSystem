# 37장: Raspberry Pi 사용자 공간 프로그램
#   Pi에서 바로:      make
#   PC에서 교차 빌드: make CROSS=aarch64-linux-gnu-      (64비트 Raspberry Pi OS)
CROSS  ?=
CC      = $(CROSS)gcc
CFLAGS  = -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -O2 -I../ch15-i2c/common

all: gpio_button_led bmp280_read

gpio_button_led: gpio_button_led.c
	$(CC) $(CFLAGS) -o $@ $<

bmp280_read: bmp280_read.c i2c_linux.c ../ch15-i2c/common/bmp280.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f gpio_button_led bmp280_read

.PHONY: all clean
