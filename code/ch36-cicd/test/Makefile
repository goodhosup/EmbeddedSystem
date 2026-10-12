# 36장: 크래시 기록 PC 테스트 (32장의 fake_flash, crc32를 함께 쓴다)
CC     ?= gcc
CH32    = ../../ch32-bootloader
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../crashlog -I$(CH32)/common -I$(CH32)/test
SRCS    = test_crashlog.c ../crashlog/crashlog.c $(CH32)/common/crc32.c $(CH32)/test/fake_flash.c

test: test_crashlog
	./test_crashlog

test_crashlog: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f test_crashlog

.PHONY: test clean
