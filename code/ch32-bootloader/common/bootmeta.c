#include <string.h>

#include "bootmeta.h"
#include "crc32.h"
#include "flash_if.h"
#include "layout.h"

#define META_MAGIC 0x31544D42u          /* 'B' 'M' 'T' '1' */
#define REC_SIZE   16u
#define RECS       (META_SECTOR_SIZE / REC_SIZE)

typedef struct {
    uint32_t     magic;
    uint32_t     seq;
    boot_state_t st;
    uint32_t     crc;                   /* 앞 12 B의 CRC-32 */
} meta_rec_t;

static const uint32_t sector[2] = { META0_ADDR, META1_ADDR };

static const meta_rec_t *rec_at(int s, uint32_t i)
{
    return (const meta_rec_t *)flash_map(sector[s] + i * REC_SIZE);
}

static int rec_valid(const meta_rec_t *r)
{
    return r->magic == META_MAGIC && crc32_update(0, r, 12) == r->crc;
}

static int rec_erased(const meta_rec_t *r)
{
    const uint32_t *w = (const uint32_t *)r;
    return w[0] == 0xFFFFFFFFu && w[1] == 0xFFFFFFFFu && w[2] == 0xFFFFFFFFu && w[3] == 0xFFFFFFFFu;
}

/* 가장 큰 seq의 올바른 기록과 그 섹터, 그 섹터의 첫 빈 칸을 찾는다 */
static const meta_rec_t *find_latest(int *sec_out, uint32_t *free_out)
{
    const meta_rec_t *best = NULL;
    int best_sec = 0;
    for (int s = 0; s < 2; s++) {
        for (uint32_t i = 0; i < RECS; i++) {
            const meta_rec_t *r = rec_at(s, i);
            if (rec_erased(r)) {
                break;                              /* 덧붙이기만 하므로 빈 칸 뒤에는 기록이 없다 */
            }
            if (rec_valid(r) && (best == NULL || (int32_t)(r->seq - best->seq) > 0)) {
                best = r;
                best_sec = s;
            }
        }
    }
    if (sec_out) *sec_out = best_sec;
    if (free_out) {
        uint32_t i = 0;
        while (i < RECS && !rec_erased(rec_at(best_sec, i))) {   /* 깨진 기록도 칸을 차지한다 */
            i++;
        }
        *free_out = i;
    }
    return best;
}

int meta_load(boot_state_t *st)
{
    const meta_rec_t *r = find_latest(NULL, NULL);
    if (r == NULL) {
        st->active = 0;
        st->pending = SLOT_NONE;
        st->tries = 0;
        st->reserved = 0xFF;
        return -1;
    }
    *st = r->st;
    return 0;
}

int meta_save(const boot_state_t *st)
{
    int s;
    uint32_t free_idx;
    const meta_rec_t *latest = find_latest(&s, &free_idx);

    meta_rec_t r;
    r.magic = META_MAGIC;
    r.seq = latest ? latest->seq + 1u : 1u;
    r.st = *st;
    r.crc = crc32_update(0, &r, 12);

    if (free_idx >= RECS) {                         /* 가득 참: 다른 섹터를 지우고 거기에 */
        s ^= 1;
        if (flash_erase(sector[s]) != 0) {
            return -1;
        }
        free_idx = 0;
    }
    return flash_write(sector[s] + free_idx * REC_SIZE, &r, sizeof r);
}

int meta_erase_all(void)
{
    return (flash_erase(META0_ADDR) == 0 && flash_erase(META1_ADDR) == 0) ? 0 : -1;
}
