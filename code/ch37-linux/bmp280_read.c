/* 37장: 15장의 bmp280.c를 Raspberry Pi에서 그대로 쓴다 */
#include <stdio.h>
#include <unistd.h>

#include "bmp280.h"
#include "i2c.h"

int main(void)
{
    bmp280_t dev;
    i2c_init(100000);
    int r = bmp280_init(&dev, BMP280_ADDR_LOW);
    if (r != 0) {
        fprintf(stderr, "BMP280 not found at 0x76 (error %d): wiring? i2cdetect -y 1\n", r);
        return 1;
    }
    printf("chip id 0x%02X\n", dev.chip_id);
    for (int i = 0; i < 5; i++) {
        int32_t t;
        if (bmp280_read_temp_c100(&dev, &t) == 0) {
            printf("%d.%02d C\n", (int)(t / 100), (int)((t < 0 ? -t : t) % 100));
        }
        sleep(1);
    }
    return 0;
}
