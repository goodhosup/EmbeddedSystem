/*
 * 32장: 펌웨어 이미지 머리말
 *   슬롯 시작에 512 B 머리말, 바로 뒤(+0x200)에 앱의 벡터 테이블과 코드.
 *   tools/mkimage.py가 빌드된 .bin 앞에 붙인다. 33장에서 서명을 더한다.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#define IMG_MAGIC     0x31474D49u     /* 바이트 순서로 'I' 'M' 'G' '1' */
#define IMG_HDR_SIZE  0x200u          /* 벡터 테이블 정렬(512 B)을 맞추려고 512 */

typedef struct {
    uint32_t magic;
    uint32_t hdr_size;
    uint32_t load_addr;     /* 이 이미지가 링크된 슬롯 주소: 다른 슬롯에 쓰면 실행할 수 없다 */
    uint32_t img_size;      /* 머리말 뒤의 바이트 수 */
    uint32_t version;       /* (major << 16) | (minor << 8) | patch */
    uint32_t crc32;         /* 머리말 뒤 img_size 바이트의 CRC-32 */
    uint32_t flags;
    uint32_t hdr_crc;       /* 위 7개 필드(28 B)의 CRC-32 */
} image_hdr_t;

typedef enum {
    IMG_OK = 0,
    IMG_BAD_MAGIC,          /* 비어 있거나 이미지가 아님 */
    IMG_BAD_HDR_CRC,
    IMG_WRONG_SLOT,
    IMG_TOO_BIG,
    IMG_BAD_CRC,            /* 내용이 망가짐 (쓰다가 끊김 등) */
    IMG_BAD_VECTOR,         /* 초기 SP나 리셋 벡터가 말이 안 됨 */
} img_result_t;

img_result_t image_check(int slot, image_hdr_t *out);
const char  *image_result_str(img_result_t r);

#endif
