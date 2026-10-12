/*
 * 37장: 15장의 i2c.h를 리눅스의 /dev/i2c-1로 구현한다
 *   → 15장의 bmp280.c를 한 줄도 고치지 않고 Raspberry Pi에서 쓴다 (24장의 HAL 원칙)
 *   커널의 i2c-dev 드라이버에 ioctl(I2C_RDWR)로 "쓰고 이어서 읽기" 트랜잭션을 맡긴다.
 */
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "i2c.h"

#ifndef I2C_DEV
#define I2C_DEV "/dev/i2c-1"          /* 헤더 3번(SDA, GPIO2), 5번(SCL, GPIO3) 핀 */
#endif

static int fd = -1;

void i2c_init(uint32_t scl_hz)
{
    (void)scl_hz;                     /* 속도는 디바이스 트리(dtparam=i2c_arm_baudrate)가 정한다 */
    if (fd < 0) {
        fd = open(I2C_DEV, O_RDWR);
        if (fd < 0) perror(I2C_DEV);
    }
}

static int xfer(struct i2c_msg *m, int n)
{
    struct i2c_rdwr_ioctl_data d = { m, (unsigned)n };
    if (fd < 0) return I2C_ERR_BUS;
    return ioctl(fd, I2C_RDWR, &d) == n ? I2C_OK : I2C_ERR_NACK;
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t n)
{
    struct i2c_msg m = { addr, 0, (__u16)n, (__u8 *)data };
    return xfer(&m, 1);
}

int i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wn, uint8_t *rdata, size_t rn)
{
    struct i2c_msg m[2] = {
        { addr, 0, (__u16)wn, (__u8 *)wdata },
        { addr, I2C_M_RD, (__u16)rn, rdata },        /* 반복 시작 조건으로 이어서 읽기 (15장) */
    };
    return xfer(m, 2);
}

int i2c_probe(uint8_t addr)
{
    uint8_t dummy;
    struct i2c_msg m = { addr, I2C_M_RD, 1, &dummy };
    return xfer(&m, 1);
}

void i2c_bus_recover(void)
{
    /* 리눅스에서는 커널의 I2C 어댑터 드라이버가 버스 복구를 맡는다 */
}
