/*
 * 33장: 서명된 이미지 형식
 *   32장의 머리말에 보안 카운터와 ECDSA P-256 서명을 더했다.
 *   서명 대상: SHA-256( 머리말 앞 0x40 바이트 || 이미지 본문 )
 *   서명은 PC의 tools/sign_image.py가 개인 키로 만들고, 장치는 공개 키로 확인만 한다.
 */
#ifndef SECURE_IMAGE_H
#define SECURE_IMAGE_H

#include <stdint.h>

#define SIMG_MAGIC     0x31474953u     /* 'S' 'I' 'G' '1' */
#define SIMG_HDR_SIZE  0x200u
#define SIMG_SIGNED_HDR 0x40u          /* 서명에 포함되는 머리말 앞부분 */

typedef struct {
    uint32_t magic;
    uint32_t hdr_size;
    uint32_t load_addr;
    uint32_t img_size;
    uint32_t version;
    uint32_t sec_counter;      /* 롤백 방지: 장치에 기록된 값보다 작으면 거부 */
    uint32_t reserved[10];     /* 0x18 ~ 0x3F */
    uint8_t  sig[64];          /* 0x40: ECDSA 서명 r(32) || s(32), 빅 엔디언 */
} signed_hdr_t;

typedef enum {
    SV_OK = 0,
    SV_BAD_MAGIC,
    SV_BAD_SIZE,
    SV_BAD_KEY,
    SV_BAD_SIGNATURE,          /* 내용이 바뀌었거나 다른 키로 서명됨 */
    SV_ROLLBACK,               /* 서명은 맞지만 오래된(취약한) 버전 */
} sv_result_t;

/*
 * img: 머리말부터 시작하는 이미지 (Flash 주소 그대로 넘겨도 된다)
 * avail: img에서 읽을 수 있는 최대 바이트 (슬롯 크기)
 * pubkey: 비압축 공개 키 65 B (0x04 || X || Y)
 * min_counter: 장치가 기억하는 보안 카운터
 */
sv_result_t secure_verify(const uint8_t *img, uint32_t avail, const uint8_t pubkey[65], uint32_t min_counter);
const char *secure_result_str(sv_result_t r);

#endif
