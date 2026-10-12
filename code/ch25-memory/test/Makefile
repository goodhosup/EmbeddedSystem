# PC에서 실행하는 메모리 풀, 구조체 배치 테스트
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common

test: test_pool
	./test_pool

test_pool: test_pool.c ../common/pool.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_pool

.PHONY: test clean
