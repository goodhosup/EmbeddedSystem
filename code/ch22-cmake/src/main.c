/*
 * 22장 실습: CMake로 빌드한 펌웨어 (NUCLEO-F411RE)
 * 시작하면 버전과 빌드 종류를 출력하고, 1초마다 가짜 측정값의 이동 평균을 출력한다.
 */
#include <stdio.h>

#include "average.h"
#include "stm32f411_regs.h"
#include "uart2.h"
#include "version.h"

static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

int main(void)
{
    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\nch22 firmware %s (%s)\n", FW_VERSION, FW_BUILD_TYPE);

    average_t avg;
    average_init(&avg, 8);
    uint32_t last = 0;
    uint16_t fake = 1000;
    for (;;) {
        if (ms_ticks - last >= 1000u) {
            last += 1000u;
            fake = (uint16_t)(fake + 37u) % 2000u;
            printf("sample %u  average %u\n", fake, average_add(&avg, fake));
        }
    }
}
