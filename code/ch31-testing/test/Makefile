# 31장: PC에서 실행하는 Unity 테스트
#   먼저 ../get_unity.sh 로 Unity를 ../lib/unity 에 받는다
CC     ?= gcc
UNITY  ?= ../lib/unity/src
CFLAGS  = -std=c11 -Wall -Wextra -O0 -g -I. -I../src -I../../ch15-i2c/common -I$(UNITY)
# 커버리지: make coverage
COV     = --coverage

SRCS = main_host.c test_thermostat.c test_bmp280.c mock_i2c.c \
       ../src/thermostat.c ../../ch15-i2c/common/bmp280.c $(UNITY)/unity.c

test: test_all
	./test_all

test_all: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

coverage: clean
	$(CC) $(CFLAGS) $(COV) -o test_all $(SRCS)
	./test_all
	gcov -o . test_all-thermostat.gcda > /dev/null 2>&1 || true
	@gcov -n test_all-thermostat.gcno 2>/dev/null | grep -A1 "thermostat.c" | head -2

clean:
	rm -f test_all *.gcda *.gcno *.gcov

.PHONY: test coverage clean
