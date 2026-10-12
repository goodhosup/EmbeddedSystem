# PC에서 실행하는 스케줄 가능성 분석 도구
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2

all: rta

rta: rta_main.c rta.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: rta
	./rta --test

clean:
	rm -f rta

.PHONY: all test clean
