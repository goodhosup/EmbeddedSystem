/* TMP36 온도 센서 */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void    sensor_init(void);
int16_t sensor_read_c10(void);     /* 섭씨 × 10 (예: 234 = 23.4 °C) */

#endif
