#include "secure_image.h"

#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"

sv_result_t secure_verify(const uint8_t *img, uint32_t avail, const uint8_t pubkey[65], uint32_t min_counter)
{
    const signed_hdr_t *h = (const signed_hdr_t *)img;
    if (h->magic != SIMG_MAGIC || h->hdr_size != SIMG_HDR_SIZE) {
        return SV_BAD_MAGIC;
    }
    if (avail < SIMG_HDR_SIZE || h->img_size == 0 || h->img_size > avail - SIMG_HDR_SIZE) {
        return SV_BAD_SIZE;
    }

    /* 1. 해시: 머리말 앞부분과 본문을 이어서 */
    uint8_t hash[32];
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    int err = mbedtls_sha256_starts(&sha, 0);           /* 0 = SHA-256 (1이면 SHA-224) */
    if (err == 0) err = mbedtls_sha256_update(&sha, img, SIMG_SIGNED_HDR);
    if (err == 0) err = mbedtls_sha256_update(&sha, img + SIMG_HDR_SIZE, h->img_size);
    if (err == 0) err = mbedtls_sha256_finish(&sha, hash);
    mbedtls_sha256_free(&sha);
    if (err != 0) {                                     /* 하드웨어 가속기로 바꾸면 실패할 수 있다 */
        return SV_BAD_SIGNATURE;
    }

    /* 2. 서명 검증: 공개 키 Q와 (r, s) */
    sv_result_t res = SV_BAD_SIGNATURE;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point q;
    mbedtls_mpi r, s;
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) != 0 ||
        mbedtls_ecp_point_read_binary(&grp, &q, pubkey, 65) != 0 ||
        mbedtls_ecp_check_pubkey(&grp, &q) != 0) {
        res = SV_BAD_KEY;
    } else if (mbedtls_mpi_read_binary(&r, h->sig, 32) == 0 &&
               mbedtls_mpi_read_binary(&s, h->sig + 32, 32) == 0 &&
               mbedtls_ecdsa_verify(&grp, hash, sizeof hash, &q, &r, &s) == 0) {
        res = SV_OK;
    }

    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecp_point_free(&q);
    mbedtls_ecp_group_free(&grp);

    /* 3. 서명이 맞은 뒤에야 머리말의 값을 믿을 수 있다: 보안 카운터 확인 */
    if (res == SV_OK && h->sec_counter < min_counter) {
        res = SV_ROLLBACK;
    }
    return res;
}

const char *secure_result_str(sv_result_t r)
{
    static const char *const s[] = { "ok", "not a signed image", "bad size", "bad public key",
                                     "SIGNATURE INVALID", "rollback: security counter too old" };
    return (unsigned)r < sizeof s / sizeof s[0] ? s[r] : "?";
}
