# 32장: 부트로더 논리 PC 테스트 (가짜 NOR Flash)
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I. -I../common

SRCS = test_boot.c fake_flash.c ../common/crc32.c ../common/image.c ../common/bootmeta.c ../common/bootlogic.c

test: test_boot fake-B.img
	./test_boot fake-B.img

# 슬롯 B용으로 링크된 것처럼 보이는 가짜 앱을 만들어 mkimage.py로 머리말을 붙인다
fake-B.img: ../tools/mkimage.py
	python3 -c "import struct,sys; sys.stdout.buffer.write(struct.pack('<II',0x20020000,0x08040301)+bytes(range(256))*4)" > fake-B.bin
	python3 ../tools/mkimage.py fake-B.bin --slot B --version 1.1.0 -o $@

test_boot: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f test_boot fake-B.bin fake-B.img

.PHONY: test clean
