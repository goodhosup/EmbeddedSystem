/*
 * 33장: 서명 검증 PC 테스트
 *   Makefile이 먼저 키를 만들고(keygen.py) 이미지 세 개에 서명한다(sign_image.py):
 *     good.simg   올바른 키, 카운터 3       other.simg  다른 키로 서명
 *     old.simg    올바른 키, 카운터 1 (서명은 정상인 옛 버전)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "secure_image.h"
#include "pubkey.h"          /* build/pubkey.h: keygen.py가 만든 공개 키 */

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

#define SLOT 0x20000u

static uint8_t *load(const char *path)
{
    uint8_t *buf = malloc(SLOT);
    memset(buf, 0xFF, SLOT);                         /* 슬롯의 나머지는 지운 Flash처럼 0xFF */
    FILE *f = fopen(path, "rb");
    if (f == NULL) { printf("cannot open %s\n", path); exit(2); }
    size_t n = fread(buf, 1, SLOT, f);
    fclose(f);
    (void)n;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        printf("usage: test_secure good.simg other.simg old.simg\n");
        return 2;
    }
    uint8_t *good = load(argv[1]);
    uint8_t *other = load(argv[2]);
    uint8_t *old = load(argv[3]);

    CHECK(secure_verify(good, SLOT, signing_pubkey, 0) == SV_OK);
    CHECK(secure_verify(good, SLOT, signing_pubkey, 3) == SV_OK);          /* 카운터가 같으면 허용 */

    /* 본문 한 비트 */
    good[SIMG_HDR_SIZE + 10] ^= 0x01;
    CHECK(secure_verify(good, SLOT, signing_pubkey, 0) == SV_BAD_SIGNATURE);
    good[SIMG_HDR_SIZE + 10] ^= 0x01;
    /* 머리말의 버전 (서명 범위 안) */
    good[16] ^= 0x01;
    CHECK(secure_verify(good, SLOT, signing_pubkey, 0) == SV_BAD_SIGNATURE);
    good[16] ^= 0x01;
    /* 서명 자체 */
    good[0x40 + 5] ^= 0x80;
    CHECK(secure_verify(good, SLOT, signing_pubkey, 0) == SV_BAD_SIGNATURE);
    good[0x40 + 5] ^= 0x80;
    CHECK(secure_verify(good, SLOT, signing_pubkey, 0) == SV_OK);          /* 되돌리면 다시 정상 */

    CHECK(secure_verify(other, SLOT, signing_pubkey, 0) == SV_BAD_SIGNATURE);   /* 다른 키 */
    CHECK(secure_verify(old, SLOT, signing_pubkey, 0) == SV_OK);           /* 서명은 정상 */
    CHECK(secure_verify(old, SLOT, signing_pubkey, 2) == SV_ROLLBACK);     /* 하지만 너무 오래됨 */

    static uint8_t empty[SLOT];
    memset(empty, 0xFF, sizeof empty);
    CHECK(secure_verify(empty, SLOT, signing_pubkey, 0) == SV_BAD_MAGIC);

    uint8_t badkey[65] = { 0x04 };                                          /* 곡선 위의 점이 아님 */
    CHECK(secure_verify(good, SLOT, badkey, 0) == SV_BAD_KEY);

    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
