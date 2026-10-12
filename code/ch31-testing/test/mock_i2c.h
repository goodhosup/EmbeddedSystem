/*
 * 31장: i2c.h(15장)의 손으로 쓴 목(mock)
 *
 * CMock이 헤더에서 자동으로 만들어 주는 것과 같은 모양이다:
 *   "이 순서로, 이 인자로 불릴 것이다. 불리면 이 값을 돌려줘라"를 미리 등록하고,
 *   테스트가 끝나면 등록한 호출이 모두 일어났는지 확인한다.
 */
#ifndef MOCK_I2C_H
#define MOCK_I2C_H

#include <stddef.h>
#include <stdint.h>

void mock_i2c_init(void);
void mock_i2c_verify(void);       /* 등록했는데 불리지 않은 호출이 있으면 실패 */

/* i2c_write_read(addr, {reg}, 1, buf, rn) 호출을 기대하고, buf에 rdata를 채워 ret을 돌려준다 */
void i2c_write_read_ExpectAndReturn(uint8_t addr, uint8_t reg, const uint8_t *rdata, size_t rn, int ret);
/* i2c_write(addr, {reg, value}, 2) 호출을 기대한다 */
void i2c_write_ExpectAndReturn(uint8_t addr, uint8_t reg, uint8_t value, int ret);

#endif
