/* 36장: 크래시 기록의 PC 테스트 (32장의 가짜 NOR Flash 사용) */
#include <stdio.h>

#include "fake_flash.h"
#include "crashlog.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static uint32_t first_seq, last_seq, count;
static void collect(const crash_rec_t *r, void *ctx)
{
    (void)ctx;
    if (count == 0) first_seq = r->seq;
    last_seq = r->seq;
    count++;
}

int main(void)
{
    fake_flash_reset();
    CHECK(crashlog_foreach(collect, NULL) == 0);

    CHECK(crashlog_add(CRASH_HARDFAULT, 0x0102, 0x08020400, 0x08020311, 0x02000000, 3600) == 0);
    CHECK(crashlog_add(CRASH_WATCHDOG, 0x0102, 0, 0, 0, 7200) == 0);
    count = 0;
    CHECK(crashlog_foreach(collect, NULL) == 2 && first_seq == 1 && last_seq == 2);

    /* 쓰다가 전원이 끊긴 기록은 건너뛰고, 다음 기록은 그 뒤에 */
    fake_flash_fail_after(3);
    crashlog_add(CRASH_BROWNOUT, 0x0102, 0, 0, 0, 7300);
    fake_flash_power_on();
    CHECK(crashlog_add(CRASH_ASSERT, 0x0102, 0, 0, 0, 7400) == 0);
    count = 0;
    CHECK(crashlog_foreach(collect, NULL) == 3 && last_seq == 3);

    /* 가득 차면 지우고 처음부터: 4096개 + 1 */
    crashlog_clear();
    for (int i = 0; i < 4097; i++) {
        CHECK(crashlog_add(CRASH_WATCHDOG, 0x0102, 0, 0, 0, (uint32_t)i) == 0);
    }
    count = 0;
    CHECK(crashlog_foreach(collect, NULL) == 1);
    CHECK(fake_flash_erase_count(CRASHLOG_ADDR) == 2);   /* clear 한 번 + 넘침 한 번 */

    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
