CC     ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2

all: errdet

errdet: main.c
	$(CC) $(CFLAGS) -o $@ $<

run: errdet
	./errdet

clean:
	rm -f errdet

.PHONY: all run clean
