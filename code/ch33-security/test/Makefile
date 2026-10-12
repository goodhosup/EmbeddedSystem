# 33장: 서명 검증 PC 테스트
#   ../get_mbedtls.sh 로 Mbed TLS를 먼저 받는다. Python cryptography 패키지가 필요하다.
CC      ?= gcc
MBEDTLS ?= ../lib/mbedtls
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I. -Ibuild -I../common -I$(MBEDTLS)/include \
           -DMBEDTLS_CONFIG_FILE='"mbedtls_config_min.h"'

# 서명 검증에 필요한 Mbed TLS 소스만
MBED_SRCS = $(addprefix $(MBEDTLS)/library/, sha256.c bignum.c bignum_core.c ecp.c ecp_curves.c \
            ecdsa.c asn1parse.c asn1write.c platform_util.c constant_time.c)

test: build/test_secure build/good.simg build/other.simg build/old.simg
	./build/test_secure build/good.simg build/other.simg build/old.simg

build/test_secure: test_secure.c ../common/secure_image.c $(MBED_SRCS) build/pubkey.h
	$(CC) $(CFLAGS) -o $@ test_secure.c ../common/secure_image.c $(MBED_SRCS)

build:
	mkdir -p build

build/pubkey.h: | build
	python3 ../tools/keygen.py build
	python3 ../tools/keygen.py build/other

build/app.bin: | build
	python3 -c "import os,sys; sys.stdout.buffer.write(os.urandom(4096))" > $@

build/good.simg: build/app.bin build/pubkey.h
	python3 ../tools/sign_image.py $< --key build/signing_key.pem --load-addr 0x08040000 --version 1.2.0 --counter 3 -o $@
build/other.simg: build/app.bin build/pubkey.h
	python3 ../tools/sign_image.py $< --key build/other/signing_key.pem --load-addr 0x08040000 --version 1.2.0 --counter 3 -o $@
build/old.simg: build/app.bin build/pubkey.h
	python3 ../tools/sign_image.py $< --key build/signing_key.pem --load-addr 0x08040000 --version 1.0.0 --counter 1 -o $@

clean:
	rm -rf build

.PHONY: test clean
