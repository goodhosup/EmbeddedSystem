/* MCP2515 비트 타이밍 계산 호스트 테스트: make test */
#include <stdio.h>

#include "mcp2515.h"

uint32_t board_millis(void) { return 0; }
void spi_init(uint32_t hz) { (void)hz; }
uint8_t spi_xfer(uint8_t o) { (void)o; return 0; }
void spi_cs_low(void) {}
void spi_cs_high(void) {}

static int check(uint32_t osc, uint32_t br, uint8_t c1, uint8_t c2, uint8_t c3)
{
    uint8_t c[3];
    int r = mcp2515_calc_timing(osc, br, c);
    int ok = r == 0 && c[0] == c1 && c[1] == c2 && c[2] == c3;
    printf("%s %2lu MHz %7lu bps -> CNF1=0x%02X CNF2=0x%02X CNF3=0x%02X\n", ok ? "ok  " : "FAIL",
           (unsigned long)(osc / 1000000u), (unsigned long)br, c[0], c[1], c[2]);
    return ok ? 0 : 1;
}

int main(void)
{
    int fail = 0;
    /* 손으로 계산한 값 (17.5절): 8 MHz 500 kbps = 8 TQ (Sync 1, Prop 2, PS1 3, PS2 2, 샘플 75%) */
    fail += check(8000000u, 500000u, 0x00, 0x91, 0x01);
    /* 8 MHz 125 kbps = BRP 1, 16 TQ (Prop 5, PS1 6, PS2 4) */
    fail += check(8000000u, 125000u, 0x01, 0xAC, 0x03);
    /* 16 MHz 500 kbps = BRP 0, 16 TQ */
    fail += check(16000000u, 500000u, 0x00, 0xAC, 0x03);
    /* 8 MHz로 1 Mbps는 4 TQ라 만들 수 없다 */
    uint8_t c[3];
    if (mcp2515_calc_timing(8000000u, 1000000u, c) == 0) {
        printf("FAIL 8 MHz 1 Mbps는 불가능해야 함\n");
        fail++;
    } else {
        printf("ok    8 MHz 1 Mbps -> 불가 (TQ가 너무 적음)\n");
    }
    printf(fail ? "%d개 실패\n" : "모두 통과\n", fail);
    return fail ? 1 : 0;
}
