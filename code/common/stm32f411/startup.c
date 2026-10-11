/*
 * STM32F411 최소 스타트업 코드 (이 책의 STM32 예제 공통)
 *
 * 벡터 테이블의 앞부분 16개는 Cortex-M4 공통 예외다
 * (Cortex-M4 프로그래밍 매뉴얼 PM0214, 2.3.4절 "Vector table").
 * 그 뒤의 주변장치 인터럽트 86개(위치 0~85, 마지막은 SPI5)는 RM0383
 * 10.1.3절의 표 "Vector table for STM32F411xC/E"를 따른다.
 * 처리기 이름은 ST의 CMSIS 스타트업 파일과 같게 붙였다(7장).
 * 예약된 위치는 Default_Handler로 채운다.
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

/* 주변장치 인터럽트 (RM0383 10.1.3절 표의 위치 순서) */
void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SDIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

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
    WWDG_IRQHandler,           /* IRQ  0 */
    PVD_IRQHandler,            /* IRQ  1 */
    TAMP_STAMP_IRQHandler,     /* IRQ  2 */
    RTC_WKUP_IRQHandler,       /* IRQ  3 */
    FLASH_IRQHandler,          /* IRQ  4 */
    RCC_IRQHandler,            /* IRQ  5 */
    EXTI0_IRQHandler,          /* IRQ  6 */
    EXTI1_IRQHandler,          /* IRQ  7 */
    EXTI2_IRQHandler,          /* IRQ  8 */
    EXTI3_IRQHandler,          /* IRQ  9 */
    EXTI4_IRQHandler,          /* IRQ 10 */
    DMA1_Stream0_IRQHandler,   /* IRQ 11 */
    DMA1_Stream1_IRQHandler,   /* IRQ 12 */
    DMA1_Stream2_IRQHandler,   /* IRQ 13 */
    DMA1_Stream3_IRQHandler,   /* IRQ 14 */
    DMA1_Stream4_IRQHandler,   /* IRQ 15 */
    DMA1_Stream5_IRQHandler,   /* IRQ 16 */
    DMA1_Stream6_IRQHandler,   /* IRQ 17 */
    ADC_IRQHandler,            /* IRQ 18 */
    Default_Handler,           /* IRQ 19: 예약 */
    Default_Handler,           /* IRQ 20: 예약 */
    Default_Handler,           /* IRQ 21: 예약 */
    Default_Handler,           /* IRQ 22: 예약 */
    EXTI9_5_IRQHandler,        /* IRQ 23 */
    TIM1_BRK_TIM9_IRQHandler,  /* IRQ 24 */
    TIM1_UP_TIM10_IRQHandler,  /* IRQ 25 */
    TIM1_TRG_COM_TIM11_IRQHandler,/* IRQ 26 */
    TIM1_CC_IRQHandler,        /* IRQ 27 */
    TIM2_IRQHandler,           /* IRQ 28 */
    TIM3_IRQHandler,           /* IRQ 29 */
    TIM4_IRQHandler,           /* IRQ 30 */
    I2C1_EV_IRQHandler,        /* IRQ 31 */
    I2C1_ER_IRQHandler,        /* IRQ 32 */
    I2C2_EV_IRQHandler,        /* IRQ 33 */
    I2C2_ER_IRQHandler,        /* IRQ 34 */
    SPI1_IRQHandler,           /* IRQ 35 */
    SPI2_IRQHandler,           /* IRQ 36 */
    USART1_IRQHandler,         /* IRQ 37 */
    USART2_IRQHandler,         /* IRQ 38 */
    Default_Handler,           /* IRQ 39: 예약 */
    EXTI15_10_IRQHandler,      /* IRQ 40 */
    RTC_Alarm_IRQHandler,      /* IRQ 41 */
    OTG_FS_WKUP_IRQHandler,    /* IRQ 42 */
    Default_Handler,           /* IRQ 43: 예약 */
    Default_Handler,           /* IRQ 44: 예약 */
    Default_Handler,           /* IRQ 45: 예약 */
    Default_Handler,           /* IRQ 46: 예약 */
    DMA1_Stream7_IRQHandler,   /* IRQ 47 */
    Default_Handler,           /* IRQ 48: 예약 */
    SDIO_IRQHandler,           /* IRQ 49 */
    TIM5_IRQHandler,           /* IRQ 50 */
    SPI3_IRQHandler,           /* IRQ 51 */
    Default_Handler,           /* IRQ 52: 예약 */
    Default_Handler,           /* IRQ 53: 예약 */
    Default_Handler,           /* IRQ 54: 예약 */
    Default_Handler,           /* IRQ 55: 예약 */
    DMA2_Stream0_IRQHandler,   /* IRQ 56 */
    DMA2_Stream1_IRQHandler,   /* IRQ 57 */
    DMA2_Stream2_IRQHandler,   /* IRQ 58 */
    DMA2_Stream3_IRQHandler,   /* IRQ 59 */
    DMA2_Stream4_IRQHandler,   /* IRQ 60 */
    Default_Handler,           /* IRQ 61: 예약 */
    Default_Handler,           /* IRQ 62: 예약 */
    Default_Handler,           /* IRQ 63: 예약 */
    Default_Handler,           /* IRQ 64: 예약 */
    Default_Handler,           /* IRQ 65: 예약 */
    Default_Handler,           /* IRQ 66: 예약 */
    OTG_FS_IRQHandler,         /* IRQ 67 */
    DMA2_Stream5_IRQHandler,   /* IRQ 68 */
    DMA2_Stream6_IRQHandler,   /* IRQ 69 */
    DMA2_Stream7_IRQHandler,   /* IRQ 70 */
    USART6_IRQHandler,         /* IRQ 71 */
    I2C3_EV_IRQHandler,        /* IRQ 72 */
    I2C3_ER_IRQHandler,        /* IRQ 73 */
    Default_Handler,           /* IRQ 74: 예약 */
    Default_Handler,           /* IRQ 75: 예약 */
    Default_Handler,           /* IRQ 76: 예약 */
    Default_Handler,           /* IRQ 77: 예약 */
    Default_Handler,           /* IRQ 78: 예약 */
    Default_Handler,           /* IRQ 79: 예약 */
    Default_Handler,           /* IRQ 80: 예약 */
    FPU_IRQHandler,            /* IRQ 81 */
    Default_Handler,           /* IRQ 82: 예약 */
    Default_Handler,           /* IRQ 83: 예약 */
    SPI4_IRQHandler,           /* IRQ 84 */
    SPI5_IRQHandler,           /* IRQ 85 */
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
