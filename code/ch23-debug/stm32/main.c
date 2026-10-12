/*
 * 23장 실습: 일부러 폴트를 일으키고 원인을 읽기 (NUCLEO-F411RE)
 *
 * 시리얼 터미널(115200)에서 한 글자 명령:
 *   d  0으로 나누기          → UsageFault(DIVBYZERO) → HardFault (FORCED)
 *   b  없는 주소 읽기        → BusFault(PRECISERR, BFARVALID)
 *   t  Thumb 비트 없는 점프  → UsageFault(INVSTATE)
 *   s  스택 넘치게 하기      → 결과를 관찰 (25장)
 *   c  카운터 출력 (정상 동작 확인)
 */
#include <stdio.h>

#include "stm32f411_regs.h"
#include "uart2.h"

volatile uint32_t divisor = 0;
volatile uint32_t counter;

static uint32_t recurse(uint32_t n)
{
    volatile uint8_t pad[256];               /* 스택을 많이 쓰게 */
    pad[0] = (uint8_t)n;
    return n == 0 ? pad[0] : recurse(n - 1) + pad[0];
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);

    SCB_CCR |= SCB_CCR_DIV_0_TRP;            /* 0으로 나누면 폴트 (기본은 결과 0) */

    printf("\nch23 fault demo: d=div0 b=bus t=thumb s=stack c=counter\n");

    for (;;) {
        int ch = uart2_getc();
        counter++;
        switch (ch) {
        case 'd': {
            volatile uint32_t r = 100u / divisor;
            (void)r;
            break;
        }
        case 'b': {
            volatile uint32_t v = *(volatile uint32_t *)0x60000000u;   /* F411에는 외부 메모리가 없다 */
            (void)v;
            break;
        }
        case 't': {
            void (*f)(void) = (void (*)(void))0x08000200u;              /* 짝수: Thumb 비트 없음 */
            f();
            break;
        }
        case 's':
            printf("recurse: %lu\n", (unsigned long)recurse(1000));     /* 약 256 KB 이상: SRAM 128 KB를 넘는다 */
            break;
        case 'c':
            printf("counter %lu\n", (unsigned long)counter);
            break;
        default:
            break;
        }
    }
}
