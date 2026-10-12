/*
 * 21장 실습: 변수와 함수가 어디에 놓이는지, main 전에 무슨 일이 일어나는지 확인 (NUCLEO-F411RE)
 *
 * 출력 (USART2, 115200 bps)
 *   - 링커 스크립트가 만든 기호(_sdata, _ebss, _estack 등)의 주소
 *   - 여러 종류의 변수와 함수의 주소 → 어느 섹션에 있는지
 *   - 생성자 함수가 main보다 먼저 실행되었는지
 *
 * build/ch21-boot.map 파일과 arm-none-eabi-nm, arm-none-eabi-size의 결과를 함께 비교해 본다(21.7절).
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f411_regs.h"
#include "uart2.h"

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack, end;
extern void (*__init_array_start[])(void), (*__init_array_end[])(void);

uint32_t initialized = 0x12345678;          /* .data */
uint32_t zeroed;                            /* .bss */
const uint32_t table[4] = { 1, 2, 3, 4 };   /* .rodata (Flash) */
static uint32_t ctor_ran_at;                /* .bss */

static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

__attribute__((constructor)) static void before_main(void)
{
    ctor_ran_at = 0xC0DEu;                  /* main보다 먼저 실행되면 이 값이 보인다 */
}

static void where(const char *name, const void *p)
{
    uint32_t a = (uint32_t)p;
    const char *region = (a >= 0x08000000u && a < 0x08080000u) ? "Flash"
                       : (a >= 0x20000000u && a < 0x20020000u) ? "SRAM" : "?";
    printf("  %-24s 0x%08lX  %s\n", name, (unsigned long)a, region);
}

int main(void)
{
    uint32_t local = 1;                     /* 스택 */
    static uint32_t static_local = 5;       /* .data (함수 안에 있어도 전역처럼 놓인다) */

    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\nch21 boot & sections\n");
    printf("linker symbols:\n");
    where("_sidata (.data LMA)", &_sidata);
    where("_sdata  (.data VMA)", &_sdata);
    where("_edata", &_edata);
    where("_sbss", &_sbss);
    where("_ebss / end (heap)", &end);
    where("_estack (initial SP)", &_estack);
    printf("  .data %lu bytes, .bss %lu bytes, constructors %d\n",
           (unsigned long)((uint32_t)&_edata - (uint32_t)&_sdata),
           (unsigned long)((uint32_t)&_ebss - (uint32_t)&_sbss),
           (int)(__init_array_end - __init_array_start));

    printf("objects:\n");
    where("main (function)", (const void *)main);
    where("table (const)", table);
    where("initialized (.data)", &initialized);
    where("static_local (.data)", &static_local);
    where("zeroed (.bss)", &zeroed);
    where("local (stack)", &local);

    printf("values: initialized=0x%08lX zeroed=%lu static_local=%lu\n",
           (unsigned long)initialized, (unsigned long)zeroed, (unsigned long)static_local);
    printf("constructor ran before main: %s\n", ctor_ran_at == 0xC0DEu ? "yes" : "no");

    for (;;) {
    }
}
