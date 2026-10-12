/*
 * 16장 공통 응용: SPI 플래시 ID 읽기, 섹터 지우기, 페이지 경계를 넘는 쓰기, 읽어서 검증
 */
#include <stdio.h>

#include "w25q.h"

extern uint32_t board_millis(void);

#define TEST_ADDR 0x0000F0u          /* 페이지(256바이트) 경계 0x100 직전: 두 페이지에 걸친다 */
#define TEST_LEN  300u

void app_run(void)
{
    static uint8_t wbuf[TEST_LEN], rbuf[TEST_LEN];
    uint8_t id[3];

    w25q_read_jedec_id(id);
    printf("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);
    if (id[0] == 0x00 || id[0] == 0xFF) {
        printf("no flash: check wiring, CS, power (3.3 V)\n");
        for (;;) {
        }
    }

    uint32_t t0 = board_millis();
    int r = w25q_sector_erase(TEST_ADDR);
    printf("sector erase: %s (%lu ms)\n", r == W25Q_OK ? "ok" : "timeout",
           (unsigned long)(board_millis() - t0));

    for (uint32_t i = 0; i < TEST_LEN; i++) {
        wbuf[i] = (uint8_t)(i * 7u + 3u);
    }
    t0 = board_millis();
    r = w25q_write(TEST_ADDR, wbuf, TEST_LEN);
    printf("write %u bytes: %s (%lu ms)\n", (unsigned)TEST_LEN, r == W25Q_OK ? "ok" : "timeout",
           (unsigned long)(board_millis() - t0));

    w25q_read(TEST_ADDR, rbuf, TEST_LEN);
    uint32_t bad = 0;
    for (uint32_t i = 0; i < TEST_LEN; i++) {
        if (rbuf[i] != wbuf[i]) {
            bad++;
        }
    }
    printf("verify: %s (%lu mismatches)\n", bad ? "FAIL" : "ok", (unsigned long)bad);

    for (;;) {
    }
}
