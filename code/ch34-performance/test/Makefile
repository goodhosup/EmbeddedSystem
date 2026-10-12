# 34장: Q15와 FIR의 PC 테스트
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common

test: test_dsp
	./test_dsp

test_dsp: test_dsp.c ../common/dsp.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f test_dsp

.PHONY: test clean
