/*
 * 17장 공통 응용
 *   1) 루프백 모드로 자체 시험 (버스 없이 MCP2515 안에서 송수신)
 *   2) 노멀 모드: 0.5초마다 NODE_ID 식별자로 카운터를 보내고, 받은 프레임을 출력
 *   두 보드(예: 우노 NODE_ID 0x100, NUCLEO NODE_ID 0x200)를 버스로 연결하면 서로의 프레임이 보인다.
 */
#include <stdio.h>

#include "mcp2515.h"

#ifndef MCP_OSC_HZ
#define MCP_OSC_HZ 8000000u          /* 모듈의 크리스털 (8 MHz가 흔함, 16 MHz도 있음) */
#endif
#ifndef CAN_BITRATE
#define CAN_BITRATE 500000u
#endif
#ifndef NODE_ID
#define NODE_ID 0x100u
#endif

extern uint32_t board_millis(void);

static void print_frame(const char *tag, const can_frame_t *f)
{
    printf("%s id=0x%03X dlc=%u :", tag, (unsigned)f->id, (unsigned)f->dlc);
    for (uint8_t i = 0; i < f->dlc; i++) {
        printf(" %02X", f->data[i]);
    }
    printf("\n");
}

void app_run(void)
{
    can_frame_t tx = { .id = NODE_ID, .dlc = 4, .data = { 0xDE, 0xAD, 0xBE, 0xEF } };
    can_frame_t rx;

    int r = mcp2515_init(MCP_OSC_HZ, CAN_BITRATE, 1);
    if (r != MCP_OK) {
        printf("MCP2515 init failed (%d): check SPI wiring, CS, crystal\n", r);
        for (;;) {
        }
    }
    mcp2515_send(&tx);
    uint32_t t0 = board_millis();
    while (!mcp2515_receive(&rx) && board_millis() - t0 < 100u) {
    }
    if (board_millis() - t0 >= 100u) {
        printf("loopback: no frame\n");
    } else {
        print_frame("loopback:", &rx);
    }

    mcp2515_set_normal();
    printf("normal mode, %lu bps, node 0x%03X\n", (unsigned long)CAN_BITRATE, (unsigned)NODE_ID);

    uint32_t last_tx = board_millis(), last_err = last_tx;
    uint8_t counter = 0;
    for (;;) {
        uint32_t now = board_millis();
        if (now - last_tx >= 500u) {
            last_tx = now;
            tx.dlc = 1;
            tx.data[0] = counter++;
            if (mcp2515_send(&tx) == MCP_ERR_BUSY) {
                printf("tx busy (no ACK? only one node on the bus?)\n");
            }
        }
        if (mcp2515_receive(&rx)) {
            print_frame("rx", &rx);
        }
        if (now - last_err >= 5000u) {
            last_err = now;
            uint8_t tec, rec, eflg;
            mcp2515_errors(&tec, &rec, &eflg);
            printf("TEC=%u REC=%u EFLG=0x%02X\n", tec, rec, eflg);
        }
    }
}
