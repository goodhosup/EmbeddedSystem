# PC에서 실행하는 상태 머신, 이벤트 큐, 스케줄러 테스트
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I. -I../app -I../service

test: test_washer
	./test_washer

test_washer: test_washer.c ../app/washer.c ../service/evq.c ../service/sched.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_washer

.PHONY: test clean
