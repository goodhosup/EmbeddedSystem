/*
 * STM32F411 최소 스타트업 코드 (이 책의 STM32 예제 공통)
 *
 * 벡터 테이블의 앞부분 16개는 Cortex-M4 공통 예외다
 * (Cortex-M4 프로그래밍 매뉴얼 PM0214, 2.3.4절 "Vector table").
 * 그 뒤의 주변장치 인터럽트 86개(위치 0~85, 마지막은 SPI5)는 RM0383
 * "Interrupts and events" 장의 벡터 테이블 표를 따른다. [절 번호 확인 필요]
 * 지금은 모두 Default_Handler로 채워 두고, 7장에서 이름을 붙인다.
 *
 * 21장에서 이 파일이 하는 일을 자세히 설명한다.
 */
#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;

int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* 사용자가 같은 이름의 함수를 만들면 그쪽이 쓰인다 (weak alias) */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

#define NUM_IRQ 86

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + NUM_IRQ] = {
    (vector_t)&_estack,     /* 0: 초기 스택 포인터 값 */
    Reset_Handler,          /* 1: 리셋 */
    NMI_Handler,            /* 2 */
    HardFault_Handler,      /* 3 */
    MemManage_Handler,      /* 4 */
    BusFault_Handler,       /* 5 */
    UsageFault_Handler,     /* 6 */
    0, 0, 0, 0,             /* 7~10: 예약 */
    SVC_Handler,            /* 11 */
    DebugMon_Handler,       /* 12 */
    0,                      /* 13: 예약 */
    PendSV_Handler,         /* 14 */
    SysTick_Handler,        /* 15 */
    [16 ... 16 + NUM_IRQ - 1] = Default_Handler,  /* 주변장치 인터럽트 (GCC 확장 문법) */
};

void Reset_Handler(void)
{
    /* .data: Flash에 있는 초기값을 RAM으로 복사 */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) {
        *dst++ = *src++;
    }
    /* .bss: 0으로 채우기 */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0;
    }

    main();

    for (;;) {
        /* main이 끝나면 여기서 멈춘다 */
    }
}

void Default_Handler(void)
{
    for (;;) {
        /* 처리기가 없는 예외나 인터럽트: 디버거로 멈춰서 확인한다 */
    }
}
