CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common

test: test_w25q
	./test_w25q

test_w25q: test_w25q.c ../common/w25q.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_w25q

.PHONY: test clean
