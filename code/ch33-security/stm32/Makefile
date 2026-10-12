# 33장: 보드에서 서명 검증 (Mbed TLS 최소 구성)
#   ../get_mbedtls.sh          Mbed TLS 받기
#   make keys                  키 쌍 만들기 (../keys, 개인 키는 저장소에 올리지 않는다)
#   make                       공개 키를 넣어 빌드
#   make signed-test           시험용 이미지에 서명해 슬롯 B(0x0804 0000)에 올리기
TARGET     = ch33-security
MBEDTLS   ?= ../lib/mbedtls
KEYS      ?= ../keys
USE_NEWLIB = 1

SRCS  = main.c ../common/secure_image.c ../../common/stm32f411/uart2.c
SRCS += $(addprefix $(MBEDTLS)/library/, sha256.c bignum.c bignum_core.c ecp.c ecp_curves.c \
        ecdsa.c asn1parse.c asn1write.c platform_util.c constant_time.c)
CFLAGS += -I../common -I$(KEYS) -I$(MBEDTLS)/include -DMBEDTLS_CONFIG_FILE='"mbedtls_config_min.h"'

include ../../common/stm32f411/stm32.mk

keys:
	python3 ../tools/keygen.py $(KEYS)

# 시험용: 아무 데이터 4 KB에 서명 (실행할 이미지가 아니라 검증만 해 본다)
signed-test:
	python3 -c "import os,sys; sys.stdout.buffer.write(os.urandom(4096))" > build/test.bin
	python3 ../tools/sign_image.py build/test.bin --key $(KEYS)/signing_key.pem \
	        --load-addr 0x08040000 --version 1.2.0 --counter 3 -o build/test.simg
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program build/test.simg 0x08040000 verify reset exit"

.PHONY: keys signed-test
