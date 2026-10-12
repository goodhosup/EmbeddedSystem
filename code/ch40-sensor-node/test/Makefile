# 40장: 링크 프로토콜(잡음 채널 포함)과 OTA 쓰기의 PC 테스트
CC     ?= gcc
CH32    = ../../ch32-bootloader
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common -I$(CH32)/common -I$(CH32)/test

test: test_link test_ota
	./test_link
	./test_ota

test_link: test_link.c ../common/link.c
	$(CC) $(CFLAGS) -o $@ $^

test_ota: test_ota.c ../common/ota_writer.c $(CH32)/test/fake_flash.c $(CH32)/common/crc32.c \
          $(CH32)/common/bootmeta.c $(CH32)/common/bootlogic.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_link test_ota

.PHONY: test clean
