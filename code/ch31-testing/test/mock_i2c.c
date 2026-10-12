#include <string.h>

#include "unity.h"
#include "i2c.h"
#include "mock_i2c.h"

typedef struct {
    enum { EXP_WRITE, EXP_WRITE_READ } kind;
    uint8_t addr, reg, value;
    uint8_t rdata[32];
    size_t  rn;
    int     ret;
} expect_t;

static expect_t expects[16];
static int n_exp, pos;

void mock_i2c_init(void) { n_exp = pos = 0; }

void mock_i2c_verify(void)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(n_exp, pos, "expected I2C calls were not made");
}

void i2c_write_read_ExpectAndReturn(uint8_t addr, uint8_t reg, const uint8_t *rdata, size_t rn, int ret)
{
    expect_t *e = &expects[n_exp++];
    e->kind = EXP_WRITE_READ; e->addr = addr; e->reg = reg; e->rn = rn; e->ret = ret;
    if (rdata) memcpy(e->rdata, rdata, rn);
}

void i2c_write_ExpectAndReturn(uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    expect_t *e = &expects[n_exp++];
    e->kind = EXP_WRITE; e->addr = addr; e->reg = reg; e->value = value; e->ret = ret;
}

/* ---- i2c.h 구현: 드라이버는 진짜 I2C 대신 이것을 부른다 ---- */
int i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wn, uint8_t *rdata, size_t rn)
{
    TEST_ASSERT_TRUE_MESSAGE(pos < n_exp, "unexpected i2c_write_read call");
    expect_t *e = &expects[pos++];
    TEST_ASSERT_EQUAL_MESSAGE(EXP_WRITE_READ, e->kind, "expected i2c_write, got i2c_write_read");
    TEST_ASSERT_EQUAL_HEX8(e->addr, addr);
    TEST_ASSERT_EQUAL_size_t(1, wn);
    TEST_ASSERT_EQUAL_HEX8(e->reg, wdata[0]);
    TEST_ASSERT_EQUAL_size_t(e->rn, rn);
    memcpy(rdata, e->rdata, rn);
    return e->ret;
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t n)
{
    TEST_ASSERT_TRUE_MESSAGE(pos < n_exp, "unexpected i2c_write call");
    expect_t *e = &expects[pos++];
    TEST_ASSERT_EQUAL_MESSAGE(EXP_WRITE, e->kind, "expected i2c_write_read, got i2c_write");
    TEST_ASSERT_EQUAL_HEX8(e->addr, addr);
    TEST_ASSERT_EQUAL_size_t(2, n);
    TEST_ASSERT_EQUAL_HEX8(e->reg, data[0]);
    TEST_ASSERT_EQUAL_HEX8(e->value, data[1]);
    return e->ret;
}

/* bmp280.c가 쓰지 않는 함수는 불리면 실패 */
void i2c_init(uint32_t scl_hz) { (void)scl_hz; TEST_FAIL_MESSAGE("i2c_init not expected"); }
int  i2c_probe(uint8_t addr) { (void)addr; TEST_FAIL_MESSAGE("i2c_probe not expected"); return 0; }
void i2c_bus_recover(void) { TEST_FAIL_MESSAGE("i2c_bus_recover not expected"); }
