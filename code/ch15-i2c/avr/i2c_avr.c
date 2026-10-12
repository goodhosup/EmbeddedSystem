/*
 * I2C 마스터: ATmega328P TWI (15장)
 *   SDA = A4 (PC4), SCL = A5 (PC5)
 *
 * 레지스터: ATmega328P 데이터시트 2-wire Serial Interface 장
 *   TWBR, TWSR(상태 코드 TWS7:3, 분주 TWPS1:0), TWDR, TWCR(TWINT, TWEA, TWSTA, TWSTO, TWEN)
 *   SCL 주파수 = F_CPU / (16 + 2 × TWBR × 4^TWPS)
 *
 * 상태 코드(마스터): 0x08 START, 0x10 반복 START, 0x18/0x20 SLA+W ACK/NACK,
 *   0x28/0x30 데이터 송신 ACK/NACK, 0x40/0x48 SLA+R ACK/NACK, 0x50/0x58 데이터 수신 ACK/NACK 반환
 */
#include "i2c.h"

#include <avr/io.h>
#include <util/delay.h>

#define TW_STATUS (TWSR & 0xF8)

static int twi_wait(void)
{
    uint16_t t = 0;
    while ((TWCR & (1 << TWINT)) == 0) {     /* 하드웨어가 한 단계를 끝내면 TWINT = 1 */
        if (++t == 0) {
            return I2C_ERR_TIMEOUT;
        }
    }
    return I2C_OK;
}

static int twi_start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (twi_wait() != I2C_OK) {
        return I2C_ERR_TIMEOUT;
    }
    return (TW_STATUS == 0x08 || TW_STATUS == 0x10) ? I2C_OK : I2C_ERR_BUS;
}

static void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    uint16_t t = 0;
    while ((TWCR & (1 << TWSTO)) && ++t) {    /* 정지 조건을 다 보낼 때까지 */
    }
}

static int twi_send(uint8_t byte, uint8_t ack_status)
{
    TWDR = byte;
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (twi_wait() != I2C_OK) {
        return I2C_ERR_TIMEOUT;
    }
    return (TW_STATUS == ack_status) ? I2C_OK : I2C_ERR_NACK;
}

static int twi_recv(uint8_t *byte, uint8_t ack)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (ack ? (1 << TWEA) : 0);
    if (twi_wait() != I2C_OK) {
        return I2C_ERR_TIMEOUT;
    }
    *byte = TWDR;
    return I2C_OK;
}

void i2c_init(uint32_t scl_hz)
{
    PORTC |= (1 << PORTC4) | (1 << PORTC5);   /* 내부 풀업(20~50 kΩ): 보조용. 외부 4.7 kΩ 권장 */
    TWSR = 0;                                 /* 분주 1 */
    TWBR = (uint8_t)((F_CPU / scl_hz - 16u) / 2u);   /* 100 kHz → 72, 400 kHz → 12 */
    TWCR = (1 << TWEN);
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t n)
{
    int r = twi_start();
    if (r == I2C_OK) {
        r = twi_send((uint8_t)(addr << 1), 0x18);
    }
    for (size_t i = 0; r == I2C_OK && i < n; i++) {
        r = twi_send(data[i], 0x28);
    }
    twi_stop();
    return r;
}

int i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wn, uint8_t *rdata, size_t rn)
{
    int r = twi_start();
    if (r == I2C_OK) {
        r = twi_send((uint8_t)(addr << 1), 0x18);
    }
    for (size_t i = 0; r == I2C_OK && i < wn; i++) {
        r = twi_send(wdata[i], 0x28);
    }
    if (r == I2C_OK) {
        r = twi_start();                                  /* 반복 시작 */
    }
    if (r == I2C_OK) {
        r = twi_send((uint8_t)((addr << 1) | 1u), 0x40);   /* SLA+R */
    }
    for (size_t i = 0; r == I2C_OK && i < rn; i++) {
        r = twi_recv(&rdata[i], i + 1 < rn);              /* 마지막 바이트에는 NACK */
    }
    twi_stop();
    return r;
}

int i2c_probe(uint8_t addr)
{
    int r = twi_start();
    if (r == I2C_OK) {
        r = twi_send((uint8_t)(addr << 1), 0x18);
    }
    twi_stop();
    return r;
}

void i2c_bus_recover(void)
{
    TWCR = 0;                                 /* TWI를 끄고 핀을 직접 다룬다 */
    DDRC &= ~((1 << DDC4) | (1 << DDC5));
    PORTC &= ~(1 << PORTC5);                  /* SCL: DDR로 Low/놓기 (오픈 드레인 흉내, 2장) */
    for (uint8_t i = 0; i < 9 && !(PINC & (1 << PINC4)); i++) {
        DDRC |= (1 << DDC5);                  /* SCL Low */
        _delay_us(5);
        DDRC &= ~(1 << DDC5);                 /* SCL 놓기 */
        _delay_us(5);
    }
    PORTC |= (1 << PORTC4) | (1 << PORTC5);
    TWCR = (1 << TWEN);
    twi_stop();
}
