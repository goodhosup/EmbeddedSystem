CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common -I../../ch16-spi/common

test: test_timing
	./test_timing

test_timing: test_timing.c ../common/mcp2515.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_timing

.PHONY: test clean
