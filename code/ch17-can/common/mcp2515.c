/*
 * MCP2515 CAN 컨트롤러 드라이버
 *
 * 명령과 레지스터: Microchip MCP2515 데이터시트
 *   SPI 명령: RESET 0xC0, READ 0x03, WRITE 0x02, BIT MODIFY 0x05, RTS(TXB0) 0x81
 *   CANSTAT 0x0E, CANCTRL 0x0F (REQOP[7:5]: 000 노멀, 010 루프백, 100 설정)
 *   CNF3 0x28, CNF2 0x29, CNF1 0x2A, CANINTF 0x2C (RX0IF bit0, RX1IF bit1), EFLG 0x2D
 *   TEC 0x1C, REC 0x1D
 *   TXB0CTRL 0x30 (TXREQ bit3), TXB0SIDH 0x31, TXB0SIDL 0x32, TXB0DLC 0x35, TXB0D0 0x36
 *   RXB0CTRL 0x60, RXB0SIDH 0x61, RXB0SIDL 0x62, RXB0DLC 0x65, RXB0D0 0x66 (RXB1은 +0x10)
 *
 * 비트 타이밍: TQ = 2 × (BRP + 1) / Fosc, 한 비트 = Sync(1) + Prop + PS1 + PS2 [TQ]
 *   CNF1 = (SJW − 1) << 6 | BRP
 *   CNF2 = BTLMODE(1) << 7 | SAM << 6 | (PS1 − 1) << 3 | (Prop − 1)
 *   CNF3 = PS2 − 1
 */
#include "mcp2515.h"
#include "spi.h"

extern uint32_t board_millis(void);

#define CMD_RESET   0xC0u
#define CMD_READ    0x03u
#define CMD_WRITE   0x02u
#define CMD_MODIFY  0x05u
#define CMD_RTS_TX0 0x81u

#define REG_CANSTAT  0x0Eu
#define REG_CANCTRL  0x0Fu
#define REG_TEC      0x1Cu
#define REG_REC      0x1Du
#define REG_CNF3     0x28u
#define REG_CNF2     0x29u
#define REG_CNF1     0x2Au
#define REG_CANINTE  0x2Bu
#define REG_CANINTF  0x2Cu
#define REG_EFLG     0x2Du
#define REG_TXB0CTRL 0x30u
#define REG_TXB0SIDH 0x31u
#define REG_RXB0CTRL 0x60u
#define REG_RXB1CTRL 0x70u

#define MODE_NORMAL   0x00u
#define MODE_LOOPBACK 0x40u
#define MODE_CONFIG   0x80u

static uint8_t read_reg(uint8_t a)
{
    spi_cs_low();
    spi_xfer(CMD_READ);
    spi_xfer(a);
    uint8_t v = spi_xfer(0x00);
    spi_cs_high();
    return v;
}

static void write_regs(uint8_t a, const uint8_t *v, uint8_t n)
{
    spi_cs_low();
    spi_xfer(CMD_WRITE);
    spi_xfer(a);
    for (uint8_t i = 0; i < n; i++) {
        spi_xfer(v[i]);
    }
    spi_cs_high();
}

static void write_reg(uint8_t a, uint8_t v)
{
    write_regs(a, &v, 1);
}

static void modify(uint8_t a, uint8_t mask, uint8_t v)     /* 마스크의 1인 비트만 바꾼다 */
{
    spi_cs_low();
    spi_xfer(CMD_MODIFY);
    spi_xfer(a);
    spi_xfer(mask);
    spi_xfer(v);
    spi_cs_high();
}

static int set_mode(uint8_t mode)
{
    modify(REG_CANCTRL, 0xE0u, mode);
    uint32_t t0 = board_millis();
    while ((read_reg(REG_CANSTAT) & 0xE0u) != mode) {       /* OPMOD가 바뀔 때까지 */
        if (board_millis() - t0 > 10u) {
            return MCP_ERR_MODE;
        }
    }
    return MCP_OK;
}

int mcp2515_calc_timing(uint32_t osc_hz, uint32_t bitrate, uint8_t cnf[3])
{
    for (uint32_t brp = 0; brp < 64; brp++) {
        uint32_t div = 2u * (brp + 1u) * bitrate;
        if (osc_hz % div != 0) {
            continue;
        }
        uint32_t n = osc_hz / div;                  /* 한 비트의 TQ 수 */
        if (n < 8 || n > 16) {
            continue;                               /* 8~16 TQ만 쓴다 (작을수록 BRP가 크다) */
        }
        uint32_t ps2 = n / 4 < 2 ? 2 : n / 4;       /* 샘플 지점 약 75% */
        uint32_t rest = n - 1u - ps2;               /* Prop + PS1 */
        uint32_t prop = rest / 2u;
        uint32_t ps1 = rest - prop;
        if (prop < 1 || prop > 8 || ps1 < 1 || ps1 > 8 || ps2 > 8) {
            continue;
        }
        cnf[0] = (uint8_t)((0u << 6) | brp);                       /* SJW = 1 TQ */
        cnf[1] = (uint8_t)(0x80u | ((ps1 - 1u) << 3) | (prop - 1u)); /* BTLMODE = 1, SAM = 0 */
        cnf[2] = (uint8_t)(ps2 - 1u);
        return MCP_OK;
    }
    return MCP_ERR_TIMING;
}

int mcp2515_init(uint32_t osc_hz, uint32_t bitrate, int loopback)
{
    uint8_t cnf[3];
    if (mcp2515_calc_timing(osc_hz, bitrate, cnf) != MCP_OK) {
        return MCP_ERR_TIMING;
    }

    spi_cs_low();
    spi_xfer(CMD_RESET);                            /* 리셋하면 설정 모드로 시작한다 */
    spi_cs_high();
    uint32_t t0 = board_millis();
    while (board_millis() - t0 < 3u) {              /* 오실레이터가 안정될 시간 */
    }
    if (set_mode(MODE_CONFIG) != MCP_OK) {
        return MCP_ERR_MODE;
    }

    write_reg(REG_CNF1, cnf[0]);
    write_reg(REG_CNF2, cnf[1]);
    write_reg(REG_CNF3, cnf[2]);
    write_reg(REG_RXB0CTRL, 0x64u);                 /* 필터 끔(모든 프레임 수신), RXB1로 넘김 허용 */
    write_reg(REG_RXB1CTRL, 0x60u);
    write_reg(REG_CANINTE, 0x00u);                  /* 이 예제는 폴링. INT 핀은 연습 문제 */
    write_reg(REG_CANINTF, 0x00u);

    return set_mode(loopback ? MODE_LOOPBACK : MODE_NORMAL);
}

int mcp2515_set_normal(void)
{
    return set_mode(MODE_NORMAL);
}

int mcp2515_send(const can_frame_t *f)
{
    if (read_reg(REG_TXB0CTRL) & 0x08u) {           /* TXREQ: 앞 프레임이 아직 나가지 않음 */
        return MCP_ERR_BUSY;
    }
    uint8_t b[13];
    uint8_t n = f->dlc > 8 ? 8 : f->dlc;
    b[0] = (uint8_t)(f->id >> 3);                   /* SIDH: 식별자 [10:3] */
    b[1] = (uint8_t)((f->id & 0x07u) << 5);         /* SIDL: 식별자 [2:0], EXIDE = 0 */
    b[2] = 0;                                       /* EID8 */
    b[3] = 0;                                       /* EID0 */
    b[4] = n;                                       /* DLC, RTR = 0 */
    for (uint8_t i = 0; i < n; i++) {
        b[5 + i] = f->data[i];
    }
    write_regs(REG_TXB0SIDH, b, (uint8_t)(5 + n));
    spi_cs_low();
    spi_xfer(CMD_RTS_TX0);                          /* 송신 요청 */
    spi_cs_high();
    return MCP_OK;
}

int mcp2515_receive(can_frame_t *f)
{
    uint8_t intf = read_reg(REG_CANINTF);
    uint8_t base, flag;
    if (intf & 0x01u) {
        base = 0x61u;
        flag = 0x01u;
    } else if (intf & 0x02u) {
        base = 0x71u;
        flag = 0x02u;
    } else {
        return 0;
    }
    uint8_t b[13];
    spi_cs_low();
    spi_xfer(CMD_READ);
    spi_xfer(base);
    for (uint8_t i = 0; i < 13; i++) {
        b[i] = spi_xfer(0x00);
    }
    spi_cs_high();
    f->id = (uint16_t)(((uint16_t)b[0] << 3) | (b[1] >> 5));
    f->dlc = b[4] & 0x0Fu;
    if (f->dlc > 8) {
        f->dlc = 8;
    }
    for (uint8_t i = 0; i < f->dlc; i++) {
        f->data[i] = b[5 + i];
    }
    modify(REG_CANINTF, flag, 0x00u);               /* 수신 버퍼 비우기 */
    return 1;
}

void mcp2515_errors(uint8_t *tec, uint8_t *rec, uint8_t *eflg)
{
    *tec = read_reg(REG_TEC);
    *rec = read_reg(REG_REC);
    *eflg = read_reg(REG_EFLG);
}
