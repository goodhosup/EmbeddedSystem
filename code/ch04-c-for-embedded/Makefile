# PC에서 실행하는 예제 (하드웨어 필요 없음)
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2

all: pitfalls

pitfalls: main.c bits.h
	$(CC) $(CFLAGS) -o $@ main.c

run: pitfalls
	./pitfalls

# volatile 유무에 따른 어셈블리 비교 (4.4절)
asm: volatile_demo.c
	$(CC) -std=c11 -O2 -S -o volatile_no.s volatile_demo.c
	$(CC) -std=c11 -O2 -S -DUSE_VOLATILE -o volatile_yes.s volatile_demo.c
	@echo "volatile_no.s 와 volatile_yes.s 의 wait_ready 함수를 비교하세요"

clean:
	rm -f pitfalls *.s

.PHONY: all run asm clean
