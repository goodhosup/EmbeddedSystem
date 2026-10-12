/*
 * 16장 실습: SPI 플래시 (NUCLEO-F411RE)
 *
 * 연결: 플래시 VCC → 3.3 V, GND, CLK → D13(PA5), DO → D12(PA6), DI → D11(PA7), CS → D10(PB6)
 * 시리얼: ST-LINK 가상 COM 포트, 115200 bps
 */
#include <stdio.h>

#include "spi.h"
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
    spi_init(4000000);                  /* 4 MHz */

    printf("\nch16 SPI\n");
    app_run();
}
