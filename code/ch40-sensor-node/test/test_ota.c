/* 40장: OTA 쓰기 모듈 PC 테스트 (32장의 가짜 Flash와 부트 규칙을 함께 쓴다) */
#include <stdio.h>
#include <string.h>

#include "fake_flash.h"
#include "layout.h"
#include "bootmeta.h"
#include "bootlogic.h"
#include "ota_writer.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int verify_result;
static int fake_verify(uint32_t addr, uint32_t size) { (void)addr; (void)size; return verify_result; }

static uint8_t img[3000];

static void send_all(ota_t *o)
{
    for (uint32_t off = 0; off < sizeof img; off += 128) {
        uint16_t n = (uint16_t)((sizeof img - off) < 128 ? (sizeof img - off) : 128);
        CHECK(ota_chunk(o, off, &img[off], n) == OTA_OK);
    }
}

int main(void)
{
    for (size_t i = 0; i < sizeof img; i++) img[i] = (uint8_t)(i * 13 + 1);
    ota_t o;

    /* 정상: 받고, 확인하고, 시험 부팅 요청 */
    fake_flash_reset();
    ota_init(&o, fake_verify);
    verify_result = 0;
    CHECK(ota_begin(&o, 1, sizeof img, 0x010200) == OTA_OK);
    send_all(&o);
    CHECK(ota_end(&o) == OTA_OK);
    CHECK(memcmp(fake_flash_mem(SLOT_B_ADDR), img, sizeof img) == 0);   /* 마지막 조각(56 B)도 정확히 */
    boot_state_t st;
    meta_load(&st);
    CHECK(st.pending == 1 && st.tries == 0);

    /* 순서가 틀린 조각, BEGIN 없는 CHUNK */
    ota_init(&o, fake_verify);
    CHECK(ota_chunk(&o, 0, img, 128) == OTA_E_STATE);
    CHECK(ota_begin(&o, 1, sizeof img, 0x010200) == OTA_OK);
    CHECK(ota_chunk(&o, 128, &img[128], 128) == OTA_E_OFFSET);          /* 0번이 빠짐 */

    /* 모자라게 받고 END */
    CHECK(ota_chunk(&o, 0, img, 128) == OTA_OK);
    CHECK(ota_end(&o) == OTA_E_SIZE && o.state == OTA_ERROR);

    /* 서명이 틀리면 부트 상태를 건드리지 않는다 */
    fake_flash_reset();
    ota_init(&o, fake_verify);
    verify_result = -1;
    CHECK(ota_begin(&o, 1, sizeof img, 0x010200) == OTA_OK);
    send_all(&o);
    CHECK(ota_end(&o) == OTA_E_VERIFY);
    CHECK(meta_load(&st) == -1);                                        /* 기록 없음 = 기존 상태 그대로 */

    CHECK(ota_begin(&o, 2, 100, 0) == OTA_E_STATE);
    CHECK(ota_begin(&o, 0, SLOT_SIZE + 1, 0) == OTA_E_SIZE);

    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
