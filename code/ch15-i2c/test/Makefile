# PC에서 실행하는 드라이버 테스트
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I../common

test: test_bmp280
	./test_bmp280

test_bmp280: test_bmp280.c ../common/bmp280.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_bmp280

.PHONY: test clean
