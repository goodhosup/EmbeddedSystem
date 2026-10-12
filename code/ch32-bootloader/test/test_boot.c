/*
 * 32장: 부트로더 논리의 PC 테스트
 *   CRC, 이미지 검사, 부트 상태 기록(전원 차단 포함), 시험 부팅 → 확정/롤백 전체 시나리오
 */
#include <stdio.h>
#include <string.h>

#include "fake_flash.h"
#include "flash_if.h"
#include "layout.h"
#include "crc32.h"
#include "image.h"
#include "bootmeta.h"
#include "bootlogic.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

/* mkimage.py가 하는 일을 C로: 슬롯에 올바른 이미지를 만든다 */
static void put_image(int slot, uint32_t version, uint32_t link_slot_addr)
{
    uint32_t base = slot_addr(slot);
    static uint32_t body[256];
    body[0] = RAM_END;                                 /* 초기 SP */
    body[1] = link_slot_addr + IMG_HDR_SIZE + 0x101u;  /* 리셋 처리기 (Thumb) */
    for (int i = 2; i < 256; i++) body[i] = version * 1000u + (uint32_t)i;

    image_hdr_t h = { IMG_MAGIC, IMG_HDR_SIZE, link_slot_addr, sizeof body, version, 0, 0, 0 };
    h.crc32 = crc32_update(0, body, sizeof body);
    h.hdr_crc = crc32_update(0, &h, 28);
    flash_erase(base);
    flash_write(base, &h, sizeof h);
    flash_write(base + IMG_HDR_SIZE, body, sizeof body);
}

static void get_valid(bool v[2])
{
    v[0] = image_check(0, NULL) == IMG_OK;
    v[1] = image_check(1, NULL) == IMG_OK;
}

/* 부트로더가 하는 일: 읽고, 정하고, 필요하면 저장 */
static boot_decision_t do_boot(void)
{
    boot_state_t st;
    bool v[2];
    meta_load(&st);
    get_valid(v);
    boot_decision_t d = boot_decide(&st, v);
    if (d.save) {
        CHECK(meta_save(&st) == 0);
    }
    return d;
}

static void test_crc32(void)
{
    CHECK(crc32_update(0, "123456789", 9) == 0xCBF43926u);     /* CRC-32 표준 확인 값 */
    CHECK(crc32_update(crc32_update(0, "1234", 4), "56789", 5) == 0xCBF43926u);   /* 이어서 계산 */
}

static void test_image_check(void)
{
    fake_flash_reset();
    CHECK(image_check(0, NULL) == IMG_BAD_MAGIC);              /* 빈 슬롯 */
    put_image(0, 0x010000, SLOT_A_ADDR);
    image_hdr_t h;
    CHECK(image_check(0, &h) == IMG_OK && h.version == 0x010000);
    put_image(1, 0x010100, SLOT_A_ADDR);                       /* A용으로 링크한 것을 B에 */
    CHECK(image_check(1, NULL) == IMG_WRONG_SLOT);
    put_image(1, 0x010100, SLOT_B_ADDR);
    fake_flash_mem(SLOT_B_ADDR + IMG_HDR_SIZE + 100)[0] ^= 0x01;   /* 한 비트 망가뜨림 */
    CHECK(image_check(1, NULL) == IMG_BAD_CRC);
}

static void test_meta_records(void)
{
    fake_flash_reset();
    boot_state_t st;
    CHECK(meta_load(&st) == -1 && st.active == 0 && st.pending == SLOT_NONE);   /* 기록 없음: 기본값 */
    for (int i = 0; i < 2500; i++) {                           /* 섹터 하나 = 1024개: 두 번 넘김 */
        st.active = (uint8_t)(i & 1);
        st.tries = (uint8_t)(i % 7);
        CHECK(meta_save(&st) == 0);
    }
    boot_state_t r;
    CHECK(meta_load(&r) == 0 && r.active == st.active && r.tries == st.tries);
    CHECK(fake_flash_erase_count(META0_ADDR) + fake_flash_erase_count(META1_ADDR) == 2);
}

static void test_meta_power_loss(void)
{
    fake_flash_reset();
    boot_state_t st = { 0, SLOT_NONE, 0, 0xFF };
    meta_save(&st);
    boot_state_t next = { 1, SLOT_NONE, 0, 0xFF };
    for (int w = 0; w < 4; w++) {                              /* 기록의 몇 번째 워드에서 끊겨도 */
        fake_flash_fail_after(w);
        meta_save(&next);
        fake_flash_power_on();
        boot_state_t r;
        meta_load(&r);
        CHECK(r.active == 0);                                  /* 이전 상태가 그대로 */
    }
    CHECK(meta_save(&next) == 0);                              /* 그 뒤 정상 저장은 된다 */
    boot_state_t r;
    meta_load(&r);
    CHECK(r.active == 1);
}

static void test_update_confirm(void)
{
    fake_flash_reset();
    put_image(0, 0x010000, SLOT_A_ADDR);
    boot_decision_t d = do_boot();
    CHECK(d.slot == 0 && d.event == BOOT_NORMAL);              /* 처음: A */

    put_image(1, 0x010100, SLOT_B_ADDR);                       /* 앱이 새 이미지를 B에 받음 */
    boot_state_t st;
    meta_load(&st);
    boot_request(&st, 1);
    meta_save(&st);

    d = do_boot();
    CHECK(d.slot == 1 && d.event == BOOT_TRIAL);               /* 시험 부팅 */
    meta_load(&st);
    CHECK(boot_confirm(&st, 1));                               /* 새 앱이 자가 진단 후 확정 */
    meta_save(&st);

    for (int i = 0; i < 5; i++) {
        d = do_boot();
        CHECK(d.slot == 1 && d.event == BOOT_NORMAL);          /* 이제부터 B */
    }
}

static void test_rollback(void)
{
    fake_flash_reset();
    put_image(0, 0x010000, SLOT_A_ADDR);
    do_boot();
    put_image(1, 0x020000, SLOT_B_ADDR);
    boot_state_t st;
    meta_load(&st);
    boot_request(&st, 1);
    meta_save(&st);

    for (unsigned i = 0; i < BOOT_MAX_TRIES; i++) {            /* 새 앱이 확정하지 못하고 계속 리셋 */
        boot_decision_t d = do_boot();
        CHECK(d.slot == 1 && d.event == BOOT_TRIAL);
    }
    boot_decision_t d = do_boot();
    CHECK(d.slot == 0 && d.event == BOOT_ROLLBACK);            /* 네 번째: A로 돌아감 */
    d = do_boot();
    CHECK(d.slot == 0 && d.event == BOOT_NORMAL);
}

static void test_broken_download_and_recovery(void)
{
    fake_flash_reset();
    put_image(0, 0x010000, SLOT_A_ADDR);
    do_boot();
    /* B에 받다가 끊김: 머리말은 썼지만 내용이 모자람 */
    put_image(1, 0x020000, SLOT_B_ADDR);
    memset(fake_flash_mem(SLOT_B_ADDR + IMG_HDR_SIZE + 512), 0xFF, 512);
    boot_state_t st;
    meta_load(&st);
    boot_request(&st, 1);                                      /* (잘못 만든 앱이 검증 없이 요청했다고 치자) */
    meta_save(&st);
    boot_decision_t d = do_boot();
    CHECK(d.slot == 0 && d.event == BOOT_PENDING_INVALID);

    /* 확정된 A가 망가지면 B라도 */
    put_image(1, 0x020000, SLOT_B_ADDR);
    fake_flash_mem(SLOT_A_ADDR + IMG_HDR_SIZE + 4)[0] ^= 0xFF;
    d = do_boot();
    CHECK(d.slot == 1 && d.event == BOOT_RECOVERY);

    fake_flash_reset();
    d = do_boot();
    CHECK(d.slot == -1 && d.event == BOOT_NO_IMAGE);
}

/* tools/mkimage.py가 만든 파일이 C의 image_check를 통과하는가 (두 구현이 같은 형식을 쓰는지) */
static void test_mkimage_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f == NULL) return;
    fake_flash_reset();
    size_t n = fread(fake_flash_mem(SLOT_B_ADDR), 1, SLOT_SIZE, f);
    fclose(f);
    CHECK(n > IMG_HDR_SIZE);
    image_hdr_t h;
    CHECK(image_check(1, &h) == IMG_OK);
    CHECK(h.version == 0x010100);                       /* --version 1.1.0 */
}

int main(int argc, char **argv)
{
    test_crc32();
    test_image_check();
    test_meta_records();
    test_meta_power_loss();
    test_update_confirm();
    test_rollback();
    test_broken_download_and_recovery();
    if (argc > 1) {
        test_mkimage_file(argv[1]);
    }
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
