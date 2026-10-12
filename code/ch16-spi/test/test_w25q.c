/*
 * W25Q 드라이버 호스트 테스트 (PC에서 실행)
 *
 * spi.h를 "가짜 플래시"로 구현한다. 가짜 플래시는 진짜 칩의 규칙을 흉내 낸다.
 *   - Write Enable(0x06) 없이 쓰기/지우기 명령이 오면 무시한다.
 *   - 쓰기는 1 → 0만 된다 (기존 값 & 새 값).
 *   - Page Program이 페이지 끝을 넘으면 같은 페이지의 처음으로 돌아간다.
 *   - 쓰기/지우기가 끝나면 WEL이 지워진다.
 * 드라이버가 이 규칙을 지키지 않으면 검증에서 실패한다.
 *
 *   make test
 */
#include <stdio.h>
#include <string.h>

#include "spi.h"
#include "w25q.h"

#define FLASH_SIZE (64u * 1024u)

static uint8_t mem[FLASH_SIZE];
static int     cs_active, wel;
static uint8_t cmd;
static int     pos;
static uint32_t addr;
static unsigned we_count, erase_count, program_count;

uint32_t board_millis(void) { return 0; }

void spi_init(uint32_t hz) { (void)hz; }

void spi_cs_low(void)
{
    cs_active = 1;
    pos = 0;
}

void spi_cs_high(void)
{
    if (cmd == 0x20 && pos >= 4 && wel) {                  /* CS를 올리는 순간 실행 */
        memset(&mem[addr & ~(W25Q_SECTOR_SIZE - 1u)], 0xFF, W25Q_SECTOR_SIZE);
        erase_count++;
        wel = 0;
    }
    if (cmd == 0x02 && pos > 4 && wel) {
        program_count++;
        wel = 0;
    }
    cs_active = 0;
}

uint8_t spi_xfer(uint8_t out)
{
    uint8_t in = 0xFF;
    if (!cs_active) {
        return in;
    }
    if (pos == 0) {
        cmd = out;
        if (cmd == 0x06) {
            wel = 1;
            we_count++;
        }
    } else if (cmd == 0x9F && pos <= 3) {
        static const uint8_t id[3] = { 0xEF, 0x40, 0x17 };  /* W25Q64 */
        in = id[pos - 1];
    } else if (cmd == 0x05) {
        in = (uint8_t)(wel ? 0x02 : 0x00);                    /* BUSY는 항상 0 */
    } else if ((cmd == 0x03 || cmd == 0x02 || cmd == 0x20) && pos <= 3) {
        addr = (addr << 8) | out;
        if (pos == 1) {
            addr = out;
        }
    } else if (cmd == 0x03) {
        in = mem[addr % FLASH_SIZE];
        addr++;
    } else if (cmd == 0x02 && wel) {
        mem[addr % FLASH_SIZE] &= out;                         /* 1 → 0만 */
        addr = (addr & ~0xFFu) | ((addr + 1u) & 0xFFu);        /* 페이지 안에서 돌아감 */
    }
    pos++;
    return in;
}

int main(void)
{
    int fail = 0;
    uint8_t id[3], w[300], r[300];

    memset(mem, 0x00, sizeof mem);                             /* 처음에는 쓰레기 값(0) */

    w25q_read_jedec_id(id);
    if (id[0] != 0xEF || id[1] != 0x40 || id[2] != 0x17) {
        printf("FAIL: JEDEC ID\n");
        fail++;
    }

    if (w25q_sector_erase(0x0000F0u) != W25Q_OK || mem[0] != 0xFF || mem[4095] != 0xFF) {
        printf("FAIL: sector erase\n");
        fail++;
    }

    for (int i = 0; i < 300; i++) {
        w[i] = (uint8_t)(i * 7 + 3);
    }
    w25q_write(0x0000F0u, w, sizeof w);                        /* 0xF0~0x21B: 페이지 세 개에 걸침 */
    w25q_read(0x0000F0u, r, sizeof r);
    if (memcmp(w, r, sizeof w) != 0) {
        printf("FAIL: write/read verify\n");
        fail++;
    }
    if (program_count != 3) {
        printf("FAIL: page program %u번 (3번 기대: 16 + 256 + 28바이트)\n", program_count);
        fail++;
    }
    if (we_count != erase_count + program_count) {
        printf("FAIL: write enable %u번, 쓰기/지우기 %u번\n", we_count, erase_count + program_count);
        fail++;
    }

    printf(fail ? "%d개 실패\n" : "모두 통과\n", fail);
    return fail ? 1 : 0;
}
