/*
 * 33장: Mbed TLS 최소 설정 — 서명 검증(ECDSA P-256 + SHA-256)에 필요한 것만 켠다.
 * -DMBEDTLS_CONFIG_FILE='"mbedtls_config_min.h"' 로 기본 설정 대신 쓴다.
 * 켜지 않은 기능은 컴파일되지 않으므로 Flash를 크게 아낀다(TLS 전체는 수백 KB).
 */
#ifndef MBEDTLS_CONFIG_MIN_H
#define MBEDTLS_CONFIG_MIN_H

#define MBEDTLS_SHA256_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C

/* 검증만 하므로 난수 발생기가 필요 없다. 서명(개인 키)은 PC의 도구만 한다 */
#define MBEDTLS_ECP_NIST_OPTIM            /* P-256 전용 빠른 나머지 연산 */

#endif
