# 35장: PC 테스트와 정적 분석
CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wconversion -O2 -I../common
SRCS    = test_safety.c ../common/supervisor.c ../common/march.c

test: test_safety
	./test_safety

test_safety: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# 정적 분석: GCC의 분석기와 (설치되어 있다면) cppcheck
analyze:
	for f in ../common/*.c bad_examples.c; do $(CC) -std=c11 -fanalyzer -Wall -Wextra -I../common -c $$f -o /dev/null; done
	@command -v cppcheck >/dev/null && cppcheck --enable=warning,style,portability --std=c11 \
	    -I../common ../common bad_examples.c || echo "cppcheck not installed (apt install cppcheck)"

clean:
	rm -f test_safety

.PHONY: test analyze clean
