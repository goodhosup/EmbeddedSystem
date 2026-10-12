/*
 * 30장: 가짜 SD 카드. spi.h의 함수를 구현해서 sd_spi.c가 진짜 카드처럼 이야기하게 한다.
 * 명령 6바이트를 모으면 응답 바이트를 출력 큐에 넣고, 다음 spi_xfer들이 하나씩 돌려준다.
 */
#include <stdlib.h>
#include <string.h>

#include "fake_sd.h"
#include "spi.h"

enum { ST_CMD, ST_WAIT_TOKEN, ST_RX_DATA };

static uint8_t *img;
static bool present = true, cs_low, idle = true, app_cmd;
static int acmd41_count;
static uint8_t cmd[6];
static int cmd_len;
static int state = ST_CMD;
static uint32_t wr_addr;
static uint8_t rx[514];
static int rx_len;
static uint32_t writes;

static uint8_t outq[600];
static int out_head, out_len;

static uint32_t now_ms;
uint32_t sd_millis(void) { return now_ms++; }   /* 부를 때마다 1 ms가 흐른다고 친다 */

static void push(uint8_t b) { if (out_len < (int)sizeof outq) outq[(out_head + out_len++) % sizeof outq] = b; }

void fake_sd_reset(void)
{
    if (img == NULL) {
        img = calloc(FAKE_SD_SECTORS, 512);
    }
    idle = true; app_cmd = false; acmd41_count = 0;
    cmd_len = 0; state = ST_CMD; out_len = 0;
}
void fake_sd_insert(bool p) { present = p; fake_sd_reset(); }
uint32_t fake_sd_writes(void) { return writes; }
uint8_t *fake_sd_image(void) { return img; }

static void r1(uint8_t v) { push(0xFF); push(v); }       /* Ncr 한 바이트 뒤 R1 */

static void do_cmd(void)
{
    uint8_t c = cmd[0] & 0x3Fu;
    uint32_t arg = ((uint32_t)cmd[1] << 24) | ((uint32_t)cmd[2] << 16) | ((uint32_t)cmd[3] << 8) | cmd[4];
    bool was_app = app_cmd;
    app_cmd = false;
    uint8_t st = idle ? 0x01u : 0x00u;

    if (c == 0) {                                        /* CMD0: CRC 0x95 필수 */
        if (cmd[5] == 0x95u) { idle = true; acmd41_count = 0; r1(0x01); }
        return;
    }
    if (c == 8) {                                        /* CMD8: CRC 0x87 필수, R7 */
        if (cmd[5] != 0x87u) { r1(0x09); return; }       /* CRC 오류 */
        r1(st); push(0x00); push(0x00); push(0x01); push((uint8_t)arg);
        return;
    }
    if (c == 55) { app_cmd = true; r1(st); return; }
    if (c == 41 && was_app) {                            /* ACMD41: 세 번째에 준비 완료 */
        if (++acmd41_count >= 3) { idle = false; }
        r1(idle ? 0x01u : 0x00u);
        return;
    }
    if (idle) { r1(0x05); return; }                      /* 초기화 전에는 다른 명령 거부 */
    if (c == 58) { r1(0x00); push(0xC0); push(0xFF); push(0x80); push(0x00); return; }  /* CCS = 1 */
    if (c == 16) { r1(0x00); return; }
    if (c == 9) {                                        /* CSD v2.0, C_SIZE = 15 → 8 MB */
        static const uint8_t csd[16] = { 0x40, 0x0E, 0x00, 0x32, 0x5B, 0x59, 0x00, 0x00,
                                         0x00, 0x0F, 0x7F, 0x80, 0x0A, 0x40, 0x00, 0x01 };
        r1(0x00); push(0xFF); push(0xFE);
        for (int i = 0; i < 16; i++) push(csd[i]);
        push(0xFF); push(0xFF);
        return;
    }
    if (c == 17) {                                       /* 블록 읽기 (SDHC: 인자 = 블록 번호) */
        if (arg >= FAKE_SD_SECTORS) { r1(0x40); return; }   /* 주소 오류 */
        r1(0x00); push(0xFF); push(0xFF); push(0xFE);
        for (int i = 0; i < 512; i++) push(img[arg * 512u + (uint32_t)i]);
        push(0xFF); push(0xFF);
        return;
    }
    if (c == 24) {
        if (arg >= FAKE_SD_SECTORS) { r1(0x40); return; }
        r1(0x00);
        wr_addr = arg;
        state = ST_WAIT_TOKEN;
        return;
    }
    r1(0x04);                                            /* illegal command */
}

void spi_init(uint32_t sclk_hz) { (void)sclk_hz; }
void spi_cs_low(void) { cs_low = true; }
void spi_cs_high(void)
{
    cs_low = false;
    out_len = 0;                                         /* 선택이 풀리면 카드는 MISO를 놓는다 */
    cmd_len = 0;
    if (state != ST_CMD) state = ST_CMD;
}

uint8_t spi_xfer(uint8_t b)
{
    if (!present || !cs_low) {
        return 0xFF;
    }
    uint8_t ret = 0xFF;
    if (out_len > 0) {
        ret = outq[out_head];
        out_head = (out_head + 1) % (int)sizeof outq;
        out_len--;
    }
    switch (state) {
    case ST_CMD:
        if (cmd_len == 0 && (b & 0xC0u) != 0x40u) {
            break;                                       /* 명령의 시작이 아니면 무시 (0xFF 등) */
        }
        cmd[cmd_len++] = b;
        if (cmd_len == 6) {
            cmd_len = 0;
            do_cmd();
        }
        break;
    case ST_WAIT_TOKEN:
        if (b == 0xFEu) { state = ST_RX_DATA; rx_len = 0; }
        break;
    case ST_RX_DATA:
        rx[rx_len++] = b;
        if (rx_len == 514) {                             /* 데이터 512 + CRC 2 */
            memcpy(&img[wr_addr * 512u], rx, 512);
            writes++;
            push(0xE5);                                  /* 데이터 응답 xxx0 0101: 받아들임 */
            push(0x00); push(0x00); push(0x00);          /* busy: Flash에 쓰는 중 */
            state = ST_CMD;
        }
        break;
    default:
        break;
    }
    return ret;
}
