/*
 * 15장 실습: I2C 버스 스캔과 BMP280/BME280 온도 (NUCLEO-F411RE)
 *
 * 연결: 센서 VCC → 3.3 V, GND → GND, SDA → D14(PB9), SCL → D15(PB8)
 * 시리얼: ST-LINK 가상 COM 포트, 115200 bps
 */
#include <stdio.h>

#include "i2c.h"
#include "stm32f411_regs.h"
#include "uart2.h"

void app_run(void);

static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

uint32_t board_millis(void)
{
    return ms_ticks;
}

int main(void)
{
    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    i2c_init(100000);

    printf("\nch15 I2C\n");
    app_run();
}
