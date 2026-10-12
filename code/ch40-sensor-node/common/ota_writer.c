#include <string.h>

#include "ota_writer.h"
#include "flash_if.h"
#include "layout.h"
#include "bootmeta.h"
#include "bootlogic.h"

void ota_init(ota_t *o, ota_verify_fn verify)
{
    memset(o, 0, sizeof *o);
    o->state = OTA_IDLE;
    o->verify = verify;
}

ota_err_t ota_begin(ota_t *o, int target_slot, uint32_t size, uint32_t version)
{
    if (target_slot < 0 || target_slot > 1) return OTA_E_STATE;
    if (size == 0 || size > SLOT_SIZE) return OTA_E_SIZE;
    o->slot = target_slot;
    o->base = slot_addr(target_slot);
    o->size = size;
    o->version = version;
    o->next_off = 0;
    if (flash_erase(o->base) != 0) {
        o->state = OTA_ERROR;
        return OTA_E_FLASH;
    }
    o->state = OTA_RECEIVING;
    return OTA_OK;
}

ota_err_t ota_chunk(ota_t *o, uint32_t offset, const uint8_t *data, uint16_t n)
{
    if (o->state != OTA_RECEIVING) return OTA_E_STATE;
    if (offset != o->next_off || (offset & 3u) != 0) return OTA_E_OFFSET;
    if (n == 0 || offset + n > o->size) return OTA_E_SIZE;
    /* Flash는 4바이트 단위로 한 번만 쓸 수 있다: 마지막 조각이 아니면 4의 배수여야 한다
       (그렇지 않으면 다음 조각이 이미 쓴 워드를 다시 쓰게 된다) */
    if ((n & 3u) != 0 && offset + n != o->size) return OTA_E_SIZE;

    uint8_t buf[260];
    if (n > sizeof buf - 4) return OTA_E_SIZE;
    uint32_t padded = ((uint32_t)n + 3u) & ~3u;      /* Flash는 4바이트 단위 (32장) */
    memcpy(buf, data, n);
    memset(buf + n, 0xFF, padded - n);
    if (flash_write(o->base + offset, buf, padded) != 0) {
        o->state = OTA_ERROR;
        return OTA_E_FLASH;
    }
    o->next_off += n;
    return OTA_OK;
}

ota_err_t ota_end(ota_t *o)
{
    if (o->state != OTA_RECEIVING) return OTA_E_STATE;
    if (o->next_off != o->size) {
        o->state = OTA_ERROR;
        return OTA_E_SIZE;
    }
    if (o->verify(o->base, o->size) != 0) {          /* 서명이 맞지 않으면 여기서 끝: 실행하지 않는다 */
        o->state = OTA_ERROR;
        return OTA_E_VERIFY;
    }
    boot_state_t st;
    meta_load(&st);
    boot_request(&st, o->slot);
    if (meta_save(&st) != 0) {
        o->state = OTA_ERROR;
        return OTA_E_FLASH;
    }
    o->state = OTA_DONE;
    return OTA_OK;
}
