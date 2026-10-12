/* BMP280 / BME280 온도 드라이버 (15장). 기압과 습도는 연습 문제 */
#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#define BMP280_ADDR_LOW   0x76    /* SDO 핀 = GND */
#define BMP280_ADDR_HIGH  0x77    /* SDO 핀 = VDDIO */

#define BMP280_CHIP_ID    0x58
#define BME280_CHIP_ID    0x60

typedef struct {
    uint8_t  addr;
    uint8_t  chip_id;
    uint16_t dig_t1;
    int16_t  dig_t2, dig_t3;
} bmp280_t;

int bmp280_init(bmp280_t *dev, uint8_t addr);           /* 0이면 성공 */
int bmp280_read_temp_c100(bmp280_t *dev, int32_t *c100); /* 섭씨 × 100 */

#endif
