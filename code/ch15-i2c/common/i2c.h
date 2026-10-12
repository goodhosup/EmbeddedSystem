/*
 * I2C 마스터 인터페이스 (15장)
 *
 * 센서 드라이버는 이 함수들만 쓴다. AVR(i2c_avr.c)과 STM32(i2c_stm32.c)가
 * 각자 같은 이름으로 구현하므로, 드라이버 코드는 두 칩에서 그대로 쓴다.
 * 주소는 7비트 주소(예: 0x76)이고, R/W 비트는 구현이 붙인다.
 */
#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

#define I2C_OK           0
#define I2C_ERR_NACK    (-1)     /* 주소나 데이터에 ACK가 없음 */
#define I2C_ERR_TIMEOUT (-2)     /* 플래그를 기다리다 시간 초과 (버스 잠김 등) */
#define I2C_ERR_BUS     (-3)     /* 버스 오류, 중재 패배 */

void i2c_init(uint32_t scl_hz);
int  i2c_write(uint8_t addr, const uint8_t *data, size_t n);
int  i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wn, uint8_t *rdata, size_t rn);
int  i2c_probe(uint8_t addr);               /* 주소에 장치가 대답하면 I2C_OK */
void i2c_bus_recover(void);                 /* SCL을 9번 흔들어 잠긴 버스를 푼다 */

#endif
