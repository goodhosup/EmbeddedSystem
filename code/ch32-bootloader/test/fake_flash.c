#include <string.h>

#include "fake_flash.h"
#include "flash_if.h"
#include "layout.h"

static uint8_t mem[FLASH_TOTAL];
static int     budget = -1;                   /* -1: 제한 없음 */
static int     dead;
static uint32_t erases[8];

static const uint32_t sec_start[8] = { 0x08000000u, 0x08004000u, 0x08008000u, 0x0800C000u,
                                       0x08010000u, 0x08020000u, 0x08040000u, 0x08060000u };
static const uint32_t sec_size[8]  = { 16384u, 16384u, 16384u, 16384u, 65536u, 131072u, 131072u, 131072u };

void fake_flash_reset(void) { memset(mem, 0xFF, sizeof mem); budget = -1; dead = 0; memset(erases, 0, sizeof erases); }
void fake_flash_fail_after(int words) { budget = words; }
void fake_flash_power_on(void) { budget = -1; dead = 0; }
uint8_t *fake_flash_mem(uint32_t addr) { return &mem[addr - FLASH_BASE_ADDR]; }

uint32_t fake_flash_erase_count(uint32_t a)
{
    for (int i = 0; i < 8; i++) if (sec_start[i] == a) return erases[i];
    return 0;
}

int flash_erase(uint32_t a)
{
    if (dead) return -1;
    for (int i = 0; i < 8; i++) {
        if (sec_start[i] == a) {
            if (budget == 0) { dead = 1; memset(fake_flash_mem(a), 0x00, 64); return -1; }  /* 지우다 끊김: 일부만 */
            memset(fake_flash_mem(a), 0xFF, sec_size[i]);
            erases[i]++;
            return 0;
        }
    }
    return -1;                                         /* 섹터 시작 주소가 아님 */
}

int flash_write(uint32_t addr, const void *data, uint32_t len)
{
    if (dead || (addr & 3u) || (len & 3u)) return -1;
    const uint8_t *d = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i += 4) {
        if (budget == 0) { dead = 1; return -1; }
        if (budget > 0) budget--;
        uint8_t *m = fake_flash_mem(addr + i);
        for (int k = 0; k < 4; k++) {
            m[k] &= d[i + (uint32_t)k];               /* NOR: 1 → 0만 가능 */
        }
    }
    return 0;
}

const void *flash_map(uint32_t addr) { return fake_flash_mem(addr); }
