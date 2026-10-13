/*
 * 15장 공통 응용: 버스 스캔 후 BMP280/BME280 온도를 1초마다 출력
 * 보드별 main.c가 초기화를 마친 뒤 app_run()을 부른다.
 */
#include <stdio.h>

#include "bmp280.h"
#include "i2c.h"

extern uint32_t board_millis(void);

static void scan_bus(void)
{
    printf("I2C scan:");
    int found = 0;
    for (uint8_t a = 0x08; a <= 0x77; a++) {
        if (i2c_probe(a) == I2C_OK) {
            printf(" 0x%02X", a);
            found++;
        }
    }
    printf(found ? "\n" : " (none)\n");
}

static void print_c100(int32_t v)
{
    if (v < 0) {
        printf("-");
        v = -v;
    }
    printf("%ld.%02ld", (long)(v / 100), (long)(v % 100));
}

void app_run(void)
{
    bmp280_t dev;
    int r;

    printf("ch15 I2C\n");
    scan_bus();

    if ((r = bmp280_init(&dev, BMP280_ADDR_LOW)) != 0 &&
        (r = bmp280_init(&dev, BMP280_ADDR_HIGH)) != 0) {
        printf("BMP280/BME280 not found (err %d)\n", r);
        for (;;) {
        }
    }
    printf("sensor at 0x%02X, chip id 0x%02X (%s)\n", dev.addr, dev.chip_id,
           dev.chip_id == BME280_CHIP_ID ? "BME280" : "BMP280");

    uint32_t last = board_millis();
    for (;;) {
        if (board_millis() - last >= 1000u) {
            last += 1000u;
            int32_t t;
            r = bmp280_read_temp_c100(&dev, &t);
            if (r == I2C_OK) {
                printf("T = ");
                print_c100(t);
                printf(" C\n");
            } else {
                printf("read error %d, recovering bus\n", r);
                i2c_bus_recover();
            }
        }
    }
}
