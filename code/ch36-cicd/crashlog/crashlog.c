#include <stddef.h>

#include "crashlog.h"
#include "crc32.h"
#include "flash_if.h"

#define MAGIC 0x31475243u               /* 'C' 'R' 'G' '1' */
#define RECS  (CRASHLOG_SIZE / sizeof(crash_rec_t))

static const crash_rec_t *at(uint32_t i)
{
    return (const crash_rec_t *)flash_map(CRASHLOG_ADDR + i * sizeof(crash_rec_t));
}

static int erased(const crash_rec_t *r)
{
    const uint32_t *w = (const uint32_t *)r;
    for (unsigned i = 0; i < sizeof *r / 4; i++) {
        if (w[i] != 0xFFFFFFFFu) return 0;
    }
    return 1;
}

static int valid(const crash_rec_t *r)
{
    return r->magic == MAGIC && crc32_update(0, r, offsetof(crash_rec_t, crc)) == r->crc;
}

int crashlog_add(uint16_t reason, uint16_t fw_version, uint32_t pc, uint32_t lr, uint32_t cfsr, uint32_t uptime_s)
{
    uint32_t i = 0, last_seq = 0;
    while (i < RECS && !erased(at(i))) {
        if (valid(at(i))) last_seq = at(i)->seq;
        i++;
    }
    if (i >= RECS) {                    /* 가득 참: 지우고 처음부터 */
        if (flash_erase(CRASHLOG_ADDR) != 0) return -1;
        i = 0;
    }
    crash_rec_t r = { MAGIC, last_seq + 1u, reason, fw_version, pc, lr, cfsr, uptime_s, 0 };
    r.crc = crc32_update(0, &r, offsetof(crash_rec_t, crc));
    return flash_write(CRASHLOG_ADDR + i * sizeof r, &r, sizeof r);
}

int crashlog_foreach(void (*fn)(const crash_rec_t *r, void *ctx), void *ctx)
{
    int n = 0;
    for (uint32_t i = 0; i < RECS && !erased(at(i)); i++) {
        if (valid(at(i))) {             /* 쓰다가 끊긴 기록은 건너뛴다 */
            fn(at(i), ctx);
            n++;
        }
    }
    return n;
}

int crashlog_clear(void)
{
    return flash_erase(CRASHLOG_ADDR);
}
