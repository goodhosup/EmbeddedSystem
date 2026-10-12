# 40장: 센서 노드 (32장 부트로더 위의 앱, 33장 서명 확인)
#   (cd ../../ch33-security && ./get_mbedtls.sh)          Mbed TLS
#   make keys                                           ../keys 에 서명 키 (저장소에 올리지 않는다)
#   make SLOT=A VER=1.0.0                               슬롯별 링크는 32장 app과 같다
SLOT      ?= A
VER       ?= 1.0.0
TARGET     = node-$(SLOT)
CH32       = ../../ch32-bootloader
CH33       = ../../ch33-security
MBEDTLS   ?= $(CH33)/lib/mbedtls
KEYS      ?= ../keys
LDSCRIPT   = $(CH32)/stm32/app/app_slot_$(SLOT).ld
USE_NEWLIB = 1

SRCS  = main.c link_uart.c ../common/link.c ../common/ota_writer.c
SRCS += $(CH32)/stm32/flash_f4.c $(CH32)/common/crc32.c $(CH32)/common/bootmeta.c $(CH32)/common/bootlogic.c
SRCS += ../../ch15-i2c/stm32/i2c_stm32.c ../../ch15-i2c/common/bmp280.c ../../common/stm32f411/uart2.c
SRCS += $(CH33)/common/secure_image.c
SRCS += $(addprefix $(MBEDTLS)/library/, sha256.c bignum.c bignum_core.c ecp.c ecp_curves.c \
        ecdsa.c asn1parse.c asn1write.c platform_util.c constant_time.c)

CFLAGS  += -I. -I../common -I$(CH32)/common -I$(CH33)/common -I../../ch15-i2c/common -I$(KEYS) \
           -I$(MBEDTLS)/include -DMBEDTLS_CONFIG_FILE='"mbedtls_config_min.h"'
LDFLAGS += -L$(CH32)/stm32

include ../../common/stm32f411/stm32.mk

keys:
	python3 $(CH33)/tools/keygen.py $(KEYS)

# 서명된 이미지 (33장 형식): ESP32 연결부가 서버에서 받아 링크로 보낸다
signed: build/$(TARGET).bin
	python3 $(CH33)/tools/sign_image.py $< --key $(KEYS)/signing_key.pem \
	        --load-addr $(if $(filter B,$(SLOT)),0x08040000,0x08020000) --version $(VER) --counter 1 \
	        -o build/$(TARGET).simg

.PHONY: keys signed
