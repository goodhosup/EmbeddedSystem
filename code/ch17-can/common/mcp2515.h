/* MCP2515 CAN 컨트롤러 드라이버 (17장). 16장의 spi.h 위에서 동작한다 */
#ifndef MCP2515_H
#define MCP2515_H

#include <stdint.h>

typedef struct {
    uint16_t id;          /* 11비트 표준 식별자 */
    uint8_t  dlc;         /* 0~8 */
    uint8_t  data[8];
} can_frame_t;

#define MCP_OK            0
#define MCP_ERR_MODE    (-1)     /* 모드 전환 실패 (칩 없음, 배선) */
#define MCP_ERR_TIMING  (-2)     /* 이 클럭으로 그 보레이트를 만들 수 없음 */
#define MCP_ERR_BUSY    (-3)     /* 송신 버퍼가 아직 비지 않음 */

/* 비트 타이밍 계산: cnf[0..2] = CNF1, CNF2, CNF3. 성공하면 0 */
int mcp2515_calc_timing(uint32_t osc_hz, uint32_t bitrate, uint8_t cnf[3]);

int  mcp2515_init(uint32_t osc_hz, uint32_t bitrate, int loopback);
int  mcp2515_set_normal(void);
int  mcp2515_send(const can_frame_t *f);
int  mcp2515_receive(can_frame_t *f);              /* 받은 프레임이 있으면 1 */
void mcp2515_errors(uint8_t *tec, uint8_t *rec, uint8_t *eflg);

#endif
