/*
 * 5장: arm-none-eabi-gcc + OpenOCD로 빌드하고 디버깅하는 Blink (NUCLEO-F411RE)
 *
 *   make            빌드
 *   make flash      업로드 (OpenOCD)
 *   make debug      OpenOCD를 GDB 서버로 띄운다 (다른 터미널에서 GDB 접속)
 *
 * LED: LD2 = PA5 (NUCLEO-64 사용자 설명서 UM1724, LED 절)
 * 지연은 빈 루프로 만든다. 정확한 시간은 8장에서 타이머로 만든다.
 */
#include "stm32f411_regs.h"

static volatile uint32_t blink_count;   /* 디버거로 지켜볼 변수 */

static void delay(volatile uint32_t count)
{
    while (count--) {
    }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << (5 * 2))) | (1u << (5 * 2));

    for (;;) {
        GPIOA->ODR ^= (1u << 5);
        blink_count++;
        delay(500000);
    }
}
