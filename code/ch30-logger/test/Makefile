# PC에서 실행하는 로거 테스트: 가짜 SD 카드 + sd_spi + FatFs + logger
#   먼저 ../get_fatfs.sh 로 FatFs를 ../lib/fatfs 에 받는다
CC     ?= gcc
FATFS  ?= ../lib/fatfs/source
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I. -I.. -I../common -I../../ch16-spi/common -I$(FATFS)

SRCS = test_logger.c fake_sd.c ../common/sd_spi.c ../common/diskio.c \
       ../common/logrec.c ../common/logger.c $(FATFS)/ff.c

test: test_logger
	./test_logger

test_logger: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f test_logger

.PHONY: test clean
