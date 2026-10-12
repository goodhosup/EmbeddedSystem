/*
 * BMP280 / BME280 온도 드라이버
 *
 * 레지스터와 보정 계산은 Bosch BMP280 데이터시트의 메모리 맵(0x88~0xFC)과
 * "Compensation formula" 절의 정수 계산식을 따른다. BME280도 온도 부분은 같다.
 *
 *   0xD0      chip_id (BMP280 = 0x58, BME280 = 0x60)
 *   0xE0      reset (0xB6을 쓰면 리셋)
 *   0xF4      ctrl_meas: osrs_t[7:5], osrs_p[4:2], mode[1:0]
 *   0xF5      config: t_sb[7:5], filter[4:2]
 *   0xFA~0xFC temp_msb, temp_lsb, temp_xlsb[7:4] → 20비트 원시 값
 *   0x88~0x8D dig_T1(부호 없음), dig_T2, dig_T3 (리틀 엔디언 16비트)
 *
 * 이 파일은 i2c.h의 함수만 쓰므로 AVR과 STM32에서 그대로 빌드된다.
 * AVR은 int가 16비트이므로 계산은 모두 int32_t로 명시한다(4장).
 */
#include "bmp280.h"
#include "i2c.h"

#define REG_CALIB    0x88
#define REG_CHIP_ID  0xD0
#define REG_CTRL     0xF4
#define REG_CONFIG   0xF5
#define REG_TEMP     0xFA

static int read_regs(bmp280_t *dev, uint8_t reg, uint8_t *buf, uint8_t n)
{
    return i2c_write_read(dev->addr, &reg, 1, buf, n);
}

static int write_reg(bmp280_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t d[2] = { reg, value };
    return i2c_write(dev->addr, d, 2);
}

int bmp280_init(bmp280_t *dev, uint8_t addr)
{
    uint8_t c[6];
    int r;

    dev->addr = addr;
    if ((r = read_regs(dev, REG_CHIP_ID, &dev->chip_id, 1)) != I2C_OK) {
        return r;
    }
    if (dev->chip_id != BMP280_CHIP_ID && dev->chip_id != BME280_CHIP_ID) {
        return -10;                                        /* 다른 장치 */
    }
    if ((r = read_regs(dev, REG_CALIB, c, 6)) != I2C_OK) {
        return r;
    }
    dev->dig_t1 = (uint16_t)(c[0] | ((uint16_t)c[1] << 8));
    dev->dig_t2 = (int16_t)(c[2] | ((uint16_t)c[3] << 8));
    dev->dig_t3 = (int16_t)(c[4] | ((uint16_t)c[5] << 8));

    /* config: 대기 0.5 ms, 필터 끔 / ctrl_meas: 온도 ×1, 기압 ×1, 노멀 모드(연속 측정) */
    if ((r = write_reg(dev, REG_CONFIG, 0x00)) != I2C_OK) {
        return r;
    }
    return write_reg(dev, REG_CTRL, (1u << 5) | (1u << 2) | 0x03u);
}

int bmp280_read_temp_c100(bmp280_t *dev, int32_t *c100)
{
    uint8_t d[3];
    int r = read_regs(dev, REG_TEMP, d, 3);
    if (r != I2C_OK) {
        return r;
    }
    int32_t adc_t = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);

    /* 데이터시트의 32비트 정수 보정식: 결과는 0.01 °C 단위 */
    int32_t t1 = (int32_t)dev->dig_t1;
    int32_t var1 = ((((adc_t >> 3) - (t1 << 1))) * (int32_t)dev->dig_t2) >> 11;
    int32_t dt = (adc_t >> 4) - t1;
    int32_t var2 = (((dt * dt) >> 12) * (int32_t)dev->dig_t3) >> 14;
    int32_t t_fine = var1 + var2;
    *c100 = (t_fine * 5 + 128) >> 8;
    return I2C_OK;
}
