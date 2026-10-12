/*
 * BMP280 드라이버 호스트 테스트 (PC에서 실행, 15.12절, 31장)
 *
 * 진짜 I2C 대신 레지스터 배열을 흉내 내는 가짜 i2c_write/i2c_write_read를 링크한다.
 * 보정값과 원시 값은 BMP280 데이터시트의 계산 예(dig_T1 = 27504, dig_T2 = 26435,
 * dig_T3 = −1000, adc_T = 519888 → 25.08 °C)를 쓴다.
 *
 *   make test
 */
#include <stdio.h>
#include <string.h>

#include "bmp280.h"
#include "i2c.h"

static uint8_t regs[256];

int i2c_write(uint8_t addr, const uint8_t *d, size_t n)
{
    (void)addr;
    for (size_t i = 1; i < n; i++) {
        regs[(uint8_t)(d[0] + i - 1)] = d[i];
    }
    return I2C_OK;
}

int i2c_write_read(uint8_t addr, const uint8_t *w, size_t wn, uint8_t *r, size_t rn)
{
    (void)addr;
    (void)wn;
    memcpy(r, &regs[w[0]], rn);
    return I2C_OK;
}

static void put16(uint8_t reg, uint16_t v)
{
    regs[reg] = (uint8_t)(v & 0xFF);
    regs[reg + 1] = (uint8_t)(v >> 8);
}

int main(void)
{
    int fail = 0;
    bmp280_t dev;

    regs[0xD0] = BMP280_CHIP_ID;
    put16(0x88, 27504);
    put16(0x8A, (uint16_t)26435);
    put16(0x8C, (uint16_t)(int16_t)-1000);
    int32_t adc = 519888;
    regs[0xFA] = (uint8_t)(adc >> 12);
    regs[0xFB] = (uint8_t)(adc >> 4);
    regs[0xFC] = (uint8_t)((adc & 0x0F) << 4);

    if (bmp280_init(&dev, BMP280_ADDR_LOW) != 0) {
        printf("FAIL: init\n");
        fail++;
    }
    if (regs[0xF4] != 0x27) {
        printf("FAIL: ctrl_meas = 0x%02X (0x27 기대)\n", regs[0xF4]);
        fail++;
    }
    int32_t t;
    bmp280_read_temp_c100(&dev, &t);
    if (t != 2508) {
        printf("FAIL: T = %ld (2508 기대)\n", (long)t);
        fail++;
    }

    regs[0xD0] = 0x33;                       /* 엉뚱한 장치 */
    if (bmp280_init(&dev, BMP280_ADDR_LOW) == 0) {
        printf("FAIL: 다른 chip id를 받아들임\n");
        fail++;
    }

    printf(fail ? "%d개 실패\n" : "모두 통과\n", fail);
    return fail ? 1 : 0;
}
