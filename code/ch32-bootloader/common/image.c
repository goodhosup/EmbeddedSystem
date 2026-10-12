#include <stddef.h>

#include "image.h"
#include "crc32.h"
#include "flash_if.h"
#include "layout.h"

img_result_t image_check(int slot, image_hdr_t *out)
{
    uint32_t base = slot_addr(slot);
    const image_hdr_t *h = (const image_hdr_t *)flash_map(base);

    if (h->magic != IMG_MAGIC || h->hdr_size != IMG_HDR_SIZE) {
        return IMG_BAD_MAGIC;
    }
    if (crc32_update(0, h, offsetof(image_hdr_t, hdr_crc)) != h->hdr_crc) {
        return IMG_BAD_HDR_CRC;
    }
    if (h->load_addr != base) {
        return IMG_WRONG_SLOT;
    }
    if (h->img_size < 8u || h->img_size > SLOT_SIZE - IMG_HDR_SIZE) {
        return IMG_TOO_BIG;
    }
    const uint8_t *body = (const uint8_t *)flash_map(base + IMG_HDR_SIZE);
    if (crc32_update(0, body, h->img_size) != h->crc32) {
        return IMG_BAD_CRC;
    }
    /* 벡터 테이블 확인: [0] 초기 SP는 RAM 안, [1] 리셋 처리기는 이 슬롯 안이고 Thumb 비트가 1 */
    const uint32_t *vt = (const uint32_t *)body;
    uint32_t sp = vt[0], pc = vt[1];
    if (sp <= RAM_START || sp > RAM_END || (sp & 3u) != 0 ||
        (pc & 1u) == 0 || pc < base + IMG_HDR_SIZE || pc >= base + IMG_HDR_SIZE + h->img_size) {
        return IMG_BAD_VECTOR;
    }
    if (out) {
        *out = *h;
    }
    return IMG_OK;
}

const char *image_result_str(img_result_t r)
{
    static const char *const s[] = { "ok", "empty or not an image", "header CRC error", "linked for other slot",
                                     "bad size", "image CRC error", "bad vector table" };
    return (unsigned)r < sizeof s / sizeof s[0] ? s[r] : "?";
}
