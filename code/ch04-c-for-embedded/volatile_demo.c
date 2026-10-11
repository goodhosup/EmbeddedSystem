/*
 * 4.4절: volatile 유무에 따라 컴파일러가 만드는 코드 비교
 *   make asm
 * 그다음 volatile_no.s와 volatile_yes.s에서 wait_ready를 찾아 본다.
 * Cortex-M용으로 보려면 CC=arm-none-eabi-gcc CFLAGS에 -mcpu=cortex-m4 -mthumb
 */
#include <stdint.h>

#ifdef USE_VOLATILE
volatile uint8_t ready;
#else
uint8_t ready;
#endif

void isr(void)
{
    ready = 1;
}

void wait_ready(void)
{
    while (ready == 0) {
    }
}
