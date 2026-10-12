# 40장: 보안 부트로더 (32장 부트로더 + 33장 서명 확인)
#   공개 키는 ../keys/pubkey.h (stm32-node에서 make keys)
TARGET     = node-boot
CH32       = ../../ch32-bootloader
CH33       = ../../ch33-security
MBEDTLS   ?= $(CH33)/lib/mbedtls
KEYS      ?= ../keys
LDSCRIPT   = boot.ld
USE_NEWLIB = 1
SRCS       = main.c $(CH32)/stm32/flash_f4.c $(CH32)/common/crc32.c $(CH32)/common/image.c \
             $(CH32)/common/bootmeta.c $(CH32)/common/bootlogic.c ../../common/stm32f411/uart2.c \
             $(CH33)/common/secure_image.c
SRCS      += $(addprefix $(MBEDTLS)/library/, sha256.c bignum.c bignum_core.c ecp.c ecp_curves.c \
             ecdsa.c asn1parse.c asn1write.c platform_util.c constant_time.c)
CFLAGS    += -I$(CH32)/common -I$(CH33)/common -I$(KEYS) -I$(MBEDTLS)/include \
             -DMBEDTLS_CONFIG_FILE='"mbedtls_config_min.h"'
LDFLAGS   += -L$(CH32)/stm32

include ../../common/stm32f411/stm32.mk

# 크기를 줄이려고 공통 규칙의 -Og 뒤에 -Os를 붙인다 (34장)
CFLAGS += -Os
