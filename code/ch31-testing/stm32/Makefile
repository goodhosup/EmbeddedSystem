# 31장: 단위 테스트를 보드에서 실행
#   ../get_unity.sh 로 Unity를 먼저 받는다
TARGET     = ch31-target-tests
UNITY     ?= ../lib/unity/src
# Unity가 setjmp/longjmp를 쓰므로 C 라이브러리가 필요하다
USE_NEWLIB = 1

SRCS  = main.c ../test/test_thermostat.c ../test/test_bmp280.c ../test/mock_i2c.c
SRCS += ../src/thermostat.c ../../ch15-i2c/common/bmp280.c
SRCS += $(UNITY)/unity.c ../../common/stm32f411/uart2.c

CFLAGS += -DUNITY_INCLUDE_CONFIG_H -I. -I../src -I../test -I../../ch15-i2c/common -I$(UNITY)

include ../../common/stm32f411/stm32.mk
