/*
 * STM32F411 레지스터 정의 (이 책의 예제에서 쓰는 것만)
 *
 * 주소와 오프셋의 출처
 *   - 주변장치 시작 주소: RM0383 2.3절 "Memory map" 표
 *   - RCC 레지스터: RM0383 6.3절 "RCC registers"
 *   - GPIO 레지스터: RM0383 8.4절 "GPIO registers"
 *   - FLASH_ACR: RM0383 3.8절 "Flash interface registers"
 *   - PWR_CR: RM0383 5.4절 "PWR registers"
 *   - SYSCFG: RM0383 7.2절 "SYSCFG registers"
 *   - EXTI: RM0383 10.3절 "EXTI registers"
 *   - TIM2~TIM5: RM0383 13.4절 "TIM2 to TIM5 registers"
 *   - DMA: RM0383 9.5절 "DMA registers"
 *   - USART: RM0383 19.6절 "USART registers"
 *   - I2C: RM0383 18.6절 "I2C registers"
 *   - SPI: RM0383 20.5절 "SPI and I2S registers"
 *   - RTC: RM0383 17.6절 "RTC registers", 저전력 모드: RM0383 5.3절
 *   - SCB_SCR: PM0214 4.4절 "System control block"
 *   - DWT 사이클 카운터: Cortex-M4 기술 참조 매뉴얼, ARMv7-M 아키텍처 참조 매뉴얼
 *   - ADC: RM0383 11.12절 "ADC registers", 보정값 주소: DS10314 "Temperature sensor
 *     calibration values", "Internal reference voltage calibration values" 표
 *   - NVIC, SysTick, SCB: Cortex-M4 프로그래밍 매뉴얼(PM0214) 4장 "Core peripherals"
 *
 * 실무에서는 ST가 배포하는 CMSIS 디바이스 헤더(stm32f411xe.h)를 쓴다.
 * 이 파일은 그 헤더가 내부에서 하는 일을 직접 보여 주려고 만들었다.
 */
#ifndef STM32F411_REGS_H
#define STM32F411_REGS_H

#include <stdint.h>

#define __IO volatile

/* ---- RCC (0x4002 3800) ---- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t PLLCFGR;     /* 0x04 */
    __IO uint32_t CFGR;        /* 0x08 */
    __IO uint32_t CIR;         /* 0x0C */
    __IO uint32_t AHB1RSTR;    /* 0x10 */
    __IO uint32_t AHB2RSTR;    /* 0x14 */
    uint32_t      RESERVED0[2];/* 0x18, 0x1C */
    __IO uint32_t APB1RSTR;    /* 0x20 */
    __IO uint32_t APB2RSTR;    /* 0x24 */
    uint32_t      RESERVED1[2];/* 0x28, 0x2C */
    __IO uint32_t AHB1ENR;     /* 0x30 */
    __IO uint32_t AHB2ENR;     /* 0x34 */
    uint32_t      RESERVED2[2];/* 0x38, 0x3C */
    __IO uint32_t APB1ENR;     /* 0x40 */
    __IO uint32_t APB2ENR;     /* 0x44 */
    uint32_t      RESERVED3[2];/* 0x48, 0x4C */
    __IO uint32_t AHB1LPENR;   /* 0x50 */
    __IO uint32_t AHB2LPENR;   /* 0x54 */
    uint32_t      RESERVED4[2];/* 0x58, 0x5C */
    __IO uint32_t APB1LPENR;   /* 0x60 */
    __IO uint32_t APB2LPENR;   /* 0x64 */
    uint32_t      RESERVED5[2];/* 0x68, 0x6C */
    __IO uint32_t BDCR;        /* 0x70 */
    __IO uint32_t CSR;         /* 0x74 */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)0x40023800u)

/* RCC_CR */
#define RCC_CR_HSION        (1u << 0)
#define RCC_CR_HSIRDY       (1u << 1)
#define RCC_CR_HSEON        (1u << 16)
#define RCC_CR_HSERDY       (1u << 17)
#define RCC_CR_HSEBYP       (1u << 18)
#define RCC_CR_PLLON        (1u << 24)
#define RCC_CR_PLLRDY       (1u << 25)

/* RCC_PLLCFGR: PLLM[5:0], PLLN[14:6], PLLP[17:16], PLLSRC[22], PLLQ[27:24] */
#define RCC_PLLCFGR_PLLM_Pos   0
#define RCC_PLLCFGR_PLLM_Msk   (0x3Fu << 0)
#define RCC_PLLCFGR_PLLN_Pos   6
#define RCC_PLLCFGR_PLLN_Msk   (0x1FFu << 6)
#define RCC_PLLCFGR_PLLP_Pos   16
#define RCC_PLLCFGR_PLLP_Msk   (3u << 16)
#define RCC_PLLCFGR_PLLSRC_HSE (1u << 22)
#define RCC_PLLCFGR_PLLQ_Pos   24
#define RCC_PLLCFGR_PLLQ_Msk   (0xFu << 24)

/* RCC_CFGR: SW[1:0], SWS[3:2], HPRE[7:4], PPRE1[12:10], PPRE2[15:13] */
#define RCC_CFGR_SW_Msk     (3u << 0)
#define RCC_CFGR_SW_PLL     (2u << 0)
#define RCC_CFGR_SWS_Msk    (3u << 2)
#define RCC_CFGR_SWS_PLL    (2u << 2)
#define RCC_CFGR_HPRE_Msk   (0xFu << 4)
#define RCC_CFGR_PPRE1_Msk  (7u << 10)
#define RCC_CFGR_PPRE1_DIV2 (4u << 10)
#define RCC_CFGR_PPRE2_Msk  (7u << 13)

/* RCC_AHB1ENR */
#define RCC_AHB1ENR_GPIOAEN (1u << 0)
#define RCC_AHB1ENR_GPIOBEN (1u << 1)
#define RCC_AHB1ENR_GPIOCEN (1u << 2)
#define RCC_AHB1ENR_DMA1EN  (1u << 21)
#define RCC_AHB1ENR_DMA2EN  (1u << 22)

/* RCC_AHB2ENR */
#define RCC_AHB2ENR_OTGFSEN (1u << 7)

/* RCC_APB1ENR */
#define RCC_APB1ENR_TIM2EN  (1u << 0)
#define RCC_APB1ENR_TIM3EN  (1u << 1)
#define RCC_APB1ENR_TIM4EN  (1u << 2)
#define RCC_APB1ENR_TIM5EN  (1u << 3)
#define RCC_APB1ENR_USART2EN (1u << 17)
#define RCC_APB1ENR_I2C1EN  (1u << 21)
#define RCC_APB1ENR_PWREN   (1u << 28)

/* RCC_APB2ENR */
#define RCC_APB2ENR_USART1EN (1u << 4)
#define RCC_APB2ENR_USART6EN (1u << 5)
#define RCC_APB2ENR_ADC1EN   (1u << 8)
#define RCC_APB2ENR_SPI1EN   (1u << 12)
#define RCC_APB2ENR_SYSCFGEN (1u << 14)

/* RCC_BDCR (백업 영역) */
#define RCC_BDCR_RTCSEL_Msk (3u << 8)
#define RCC_BDCR_RTCSEL_LSI (2u << 8)
#define RCC_BDCR_RTCEN      (1u << 15)
#define RCC_BDCR_BDRST      (1u << 16)

/* RCC_CSR: LSI와 리셋 원인 플래그 */
#define RCC_CSR_LSION       (1u << 0)
#define RCC_CSR_LSIRDY      (1u << 1)
#define RCC_CSR_RMVF        (1u << 24)
#define RCC_CSR_BORRSTF     (1u << 25)
#define RCC_CSR_PINRSTF     (1u << 26)
#define RCC_CSR_PORRSTF     (1u << 27)
#define RCC_CSR_SFTRSTF     (1u << 28)
#define RCC_CSR_IWDGRSTF    (1u << 29)
#define RCC_CSR_WWDGRSTF    (1u << 30)
#define RCC_CSR_LPWRRSTF    (1u << 31)

/* ---- GPIO (GPIOA 0x4002 0000, 포트마다 0x400씩) ---- */
typedef struct {
    __IO uint32_t MODER;       /* 0x00 */
    __IO uint32_t OTYPER;      /* 0x04 */
    __IO uint32_t OSPEEDR;     /* 0x08 */
    __IO uint32_t PUPDR;       /* 0x0C */
    __IO uint32_t IDR;         /* 0x10 */
    __IO uint32_t ODR;         /* 0x14 */
    __IO uint32_t BSRR;        /* 0x18 */
    __IO uint32_t LCKR;        /* 0x1C */
    __IO uint32_t AFR[2];      /* 0x20, 0x24 */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40020000u)
#define GPIOB ((GPIO_TypeDef *)0x40020400u)
#define GPIOC ((GPIO_TypeDef *)0x40020800u)

/* 핀을 대체 기능 af(0~15)로 설정한다 (MODER = 10, AFRL/AFRH에 번호) */
static inline void gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    port->MODER = (port->MODER & ~(3u << (pin * 2))) | (2u << (pin * 2));
    port->AFR[pin >> 3] = (port->AFR[pin >> 3] & ~(0xFu << ((pin & 7u) * 4)))
                        | (af << ((pin & 7u) * 4));
}

/* ---- FLASH 인터페이스 (0x4002 3C00) ---- */
typedef struct {
    __IO uint32_t ACR;         /* 0x00 */
    __IO uint32_t KEYR;        /* 0x04 잠금 해제 키 (32장) */
    __IO uint32_t OPTKEYR;     /* 0x08 */
    __IO uint32_t SR;          /* 0x0C */
    __IO uint32_t CR;          /* 0x10 */
    __IO uint32_t OPTCR;       /* 0x14 */
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)0x40023C00u)

#define FLASH_ACR_LATENCY_Msk (0xFu << 0)
#define FLASH_ACR_PRFTEN      (1u << 8)
#define FLASH_ACR_ICEN        (1u << 9)
#define FLASH_ACR_DCEN        (1u << 10)

/* Flash 지우기/쓰기 (RM0383 3.5절, 3.8절): 32장 */
#define FLASH_KEY1            0x45670123u
#define FLASH_KEY2            0xCDEF89ABu
#define FLASH_SR_EOP          (1u << 0)
#define FLASH_SR_OPERR        (1u << 1)
#define FLASH_SR_WRPERR       (1u << 4)
#define FLASH_SR_PGAERR       (1u << 5)
#define FLASH_SR_PGPERR       (1u << 6)
#define FLASH_SR_PGSERR       (1u << 7)
#define FLASH_SR_RDERR        (1u << 8)
#define FLASH_SR_BSY          (1u << 16)
#define FLASH_SR_ERRORS       (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_RDERR)
#define FLASH_CR_PG           (1u << 0)
#define FLASH_CR_SER          (1u << 1)
#define FLASH_CR_SNB_Pos      3u
#define FLASH_CR_SNB_Msk      (0xFu << 3)
#define FLASH_CR_PSIZE_32     (2u << 8)      /* 32비트씩 (전원 2.7~3.6 V) */
#define FLASH_CR_STRT         (1u << 16)
#define FLASH_CR_LOCK         (1u << 31)

/* ---- 독립 워치독 IWDG (0x4000 3000): RM0383 17장. 32장, 35장 ---- */
typedef struct {
    __IO uint32_t KR;          /* 0x00 키: 0x5555 설정 허용, 0xAAAA 갱신, 0xCCCC 시작 */
    __IO uint32_t PR;          /* 0x04 분주: 0 = /4 ... 6 = /256 */
    __IO uint32_t RLR;         /* 0x08 다시 채울 값 (12비트) */
    __IO uint32_t SR;          /* 0x0C PVU, RVU: 갱신 중 */
} IWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *)0x40003000u)
#define IWDG_KEY_ACCESS       0x5555u
#define IWDG_KEY_RELOAD       0xAAAAu
#define IWDG_KEY_START        0xCCCCu

/* ---- PWR (0x4000 7000) ---- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t CSR;         /* 0x04 */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)0x40007000u)

#define PWR_CR_LPDS         (1u << 0)       /* Stop에서 저전력 레귤레이터 */
#define PWR_CR_PDDS         (1u << 1)       /* 딥 슬립 = Standby (0이면 Stop) */
#define PWR_CR_CWUF         (1u << 2)
#define PWR_CR_CSBF         (1u << 3)
#define PWR_CR_DBP          (1u << 8)       /* 백업 영역(RTC) 쓰기 허용 */
#define PWR_CR_FPDS         (1u << 9)       /* Stop에서 Flash 절전 */
#define PWR_CR_VOS_Msk      (3u << 14)
#define PWR_CR_VOS_SCALE1   (3u << 14)

/* ---- SYSCFG (0x4001 3800) ---- */
typedef struct {
    __IO uint32_t MEMRMP;      /* 0x00 */
    __IO uint32_t PMC;         /* 0x04 */
    __IO uint32_t EXTICR[4];   /* 0x08 ~ 0x14: EXTI 라인 0~15의 포트 선택 */
} SYSCFG_TypeDef;

#define SYSCFG ((SYSCFG_TypeDef *)0x40013800u)

/* EXTICR 값: 0 = 포트 A, 1 = B, 2 = C, 3 = D, 4 = E, 7 = H */

/* ---- EXTI (0x4001 3C00) ---- */
typedef struct {
    __IO uint32_t IMR;         /* 0x00 인터럽트 마스크 (1 = 허용) */
    __IO uint32_t EMR;         /* 0x04 이벤트 마스크 */
    __IO uint32_t RTSR;        /* 0x08 상승 에지 선택 */
    __IO uint32_t FTSR;        /* 0x0C 하강 에지 선택 */
    __IO uint32_t SWIER;       /* 0x10 소프트웨어 인터럽트 */
    __IO uint32_t PR;          /* 0x14 대기 플래그 (1을 써서 지움) */
} EXTI_TypeDef;

#define EXTI ((EXTI_TypeDef *)0x40013C00u)


/* ---- 범용 타이머 TIM2~TIM5 (TIM2 0x4000 0000, TIM3 0x4000 0400, TIM4 0x4000 0800, TIM5 0x4000 0C00) ---- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t SMCR;        /* 0x08 */
    __IO uint32_t DIER;        /* 0x0C */
    __IO uint32_t SR;          /* 0x10 (플래그: 0을 써서 지움) */
    __IO uint32_t EGR;         /* 0x14 */
    __IO uint32_t CCMR1;       /* 0x18 */
    __IO uint32_t CCMR2;       /* 0x1C */
    __IO uint32_t CCER;        /* 0x20 */
    __IO uint32_t CNT;         /* 0x24 */
    __IO uint32_t PSC;         /* 0x28 */
    __IO uint32_t ARR;         /* 0x2C */
    uint32_t      RESERVED0;   /* 0x30 (TIM1의 RCR 자리) */
    __IO uint32_t CCR1;        /* 0x34 */
    __IO uint32_t CCR2;        /* 0x38 */
    __IO uint32_t CCR3;        /* 0x3C */
    __IO uint32_t CCR4;        /* 0x40 */
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *)0x40000000u)   /* 32비트 카운터 */
#define TIM3 ((TIM_TypeDef *)0x40000400u)   /* 16비트 */
#define TIM4 ((TIM_TypeDef *)0x40000800u)   /* 16비트 */
#define TIM5 ((TIM_TypeDef *)0x40000C00u)   /* 32비트 */

#define TIM_CR1_CEN         (1u << 0)
#define TIM_CR1_ARPE        (1u << 7)
#define TIM_CR2_MMS_Msk     (7u << 4)
#define TIM_CR2_MMS_UPDATE  (2u << 4)       /* 업데이트 이벤트를 TRGO로 내보낸다 */
#define TIM_DIER_UIE        (1u << 0)
#define TIM_DIER_CC1IE      (1u << 1)
#define TIM_SR_UIF          (1u << 0)
#define TIM_SR_CC1IF        (1u << 1)
#define TIM_SR_CC1OF        (1u << 9)
#define TIM_EGR_UG          (1u << 0)

/* CCMR1 채널 1: 출력 비교 모드 OC1M[6:4], 프리로드 OC1PE[3], 입력 선택 CC1S[1:0], 필터 IC1F[7:4] */
#define TIM_CCMR1_CC1S_Msk    (3u << 0)
#define TIM_CCMR1_CC1S_TI1    (1u << 0)
#define TIM_CCMR1_OC1PE       (1u << 3)
#define TIM_CCMR1_OC1M_Msk    (7u << 4)
#define TIM_CCMR1_OC1M_PWM1   (6u << 4)
#define TIM_CCMR1_IC1F_Msk    (0xFu << 4)

/* CCER 채널 1: CC1E[0], CC1P[1], CC1NP[3] */
#define TIM_CCER_CC1E       (1u << 0)
#define TIM_CCER_CC1P       (1u << 1)
#define TIM_CCER_CC1NP      (1u << 3)

/* ---- DMA (DMA1 0x4002 6000, DMA2 0x4002 6400) ---- */
typedef struct {
    __IO uint32_t CR;          /* +0x00 */
    __IO uint32_t NDTR;        /* +0x04 남은 전송 개수 */
    __IO uint32_t PAR;         /* +0x08 주변장치 주소 */
    __IO uint32_t M0AR;        /* +0x0C 메모리 주소 0 */
    __IO uint32_t M1AR;        /* +0x10 메모리 주소 1 (더블 버퍼 모드) */
    __IO uint32_t FCR;         /* +0x14 FIFO 제어 */
} DMA_Stream_TypeDef;

typedef struct {
    __IO uint32_t LISR;        /* 0x00 스트림 0~3 상태 */
    __IO uint32_t HISR;        /* 0x04 스트림 4~7 상태 */
    __IO uint32_t LIFCR;       /* 0x08 스트림 0~3 플래그 지우기 (1을 쓴다) */
    __IO uint32_t HIFCR;       /* 0x0C 스트림 4~7 플래그 지우기 */
    DMA_Stream_TypeDef S[8];   /* 0x10 + 0x18 × n */
} DMA_TypeDef;

#define DMA1 ((DMA_TypeDef *)0x40026000u)
#define DMA2 ((DMA_TypeDef *)0x40026400u)

#define DMA_SxCR_EN          (1u << 0)
#define DMA_SxCR_TEIE        (1u << 2)
#define DMA_SxCR_HTIE        (1u << 3)
#define DMA_SxCR_TCIE        (1u << 4)
#define DMA_SxCR_DIR_P2M     (0u << 6)
#define DMA_SxCR_DIR_M2P     (1u << 6)
#define DMA_SxCR_DIR_M2M     (2u << 6)
#define DMA_SxCR_CIRC        (1u << 8)
#define DMA_SxCR_PINC        (1u << 9)
#define DMA_SxCR_MINC        (1u << 10)
#define DMA_SxCR_PSIZE_8     (0u << 11)
#define DMA_SxCR_PSIZE_16    (1u << 11)
#define DMA_SxCR_PSIZE_32    (2u << 11)
#define DMA_SxCR_MSIZE_8     (0u << 13)
#define DMA_SxCR_MSIZE_16    (1u << 13)
#define DMA_SxCR_MSIZE_32    (2u << 13)
#define DMA_SxCR_PL_HIGH     (2u << 16)
#define DMA_SxCR_CHSEL(n)    ((uint32_t)(n) << 25)

/* LISR/LIFCR의 스트림 0 플래그. 스트림 1은 6비트, 2는 16비트, 3은 22비트 위로 같은 배치 */
#define DMA_FEIF0            (1u << 0)
#define DMA_DMEIF0           (1u << 2)
#define DMA_TEIF0            (1u << 3)
#define DMA_HTIF0            (1u << 4)
#define DMA_TCIF0            (1u << 5)
#define DMA_ALL_FLAGS0       (0x3Du)
static inline uint32_t dma_flag_shift(uint32_t stream)
{
    static const uint8_t shift[4] = { 0u, 6u, 16u, 22u };
    return shift[stream & 3u];
}

/* ---- DWT 사이클 카운터 (34장에서 자세히) ---- */
#define DEMCR       (*(volatile uint32_t *)0xE000EDFCu)
#define DEMCR_TRCENA (1u << 24)
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004u)
#define DWT_CTRL_CYCCNTENA (1u << 0)

static inline void cycle_counter_init(void)
{
    DEMCR |= DEMCR_TRCENA;          /* 추적 기능 켜기 */
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA; /* 클럭마다 1씩 증가 */
}

/* ---- USART (USART1 0x4001 1000, USART2 0x4000 4400, USART6 0x4001 1400) ---- */
typedef struct {
    __IO uint32_t SR;          /* 0x00 상태 */
    __IO uint32_t DR;          /* 0x04 데이터 */
    __IO uint32_t BRR;         /* 0x08 보레이트 */
    __IO uint32_t CR1;         /* 0x0C */
    __IO uint32_t CR2;         /* 0x10 */
    __IO uint32_t CR3;         /* 0x14 */
    __IO uint32_t GTPR;        /* 0x18 */
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)0x40011000u)
#define USART2 ((USART_TypeDef *)0x40004400u)
#define USART6 ((USART_TypeDef *)0x40011400u)

#define USART_SR_PE         (1u << 0)
#define USART_SR_FE         (1u << 1)
#define USART_SR_NF         (1u << 2)
#define USART_SR_ORE        (1u << 3)
#define USART_SR_IDLE       (1u << 4)
#define USART_SR_RXNE       (1u << 5)
#define USART_SR_TC         (1u << 6)
#define USART_SR_TXE        (1u << 7)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_IDLEIE    (1u << 4)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_TCIE      (1u << 6)
#define USART_CR1_TXEIE     (1u << 7)
#define USART_CR1_PCE       (1u << 10)
#define USART_CR1_M         (1u << 12)
#define USART_CR1_UE        (1u << 13)
#define USART_CR1_OVER8     (1u << 15)
#define USART_CR3_DMAR      (1u << 6)
#define USART_CR3_DMAT      (1u << 7)

/* ---- I2C (I2C1 0x4000 5400, I2C2 0x4000 5800, I2C3 0x4000 5C00) ---- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t OAR1;        /* 0x08 */
    __IO uint32_t OAR2;        /* 0x0C */
    __IO uint32_t DR;          /* 0x10 */
    __IO uint32_t SR1;         /* 0x14 */
    __IO uint32_t SR2;         /* 0x18 */
    __IO uint32_t CCR;         /* 0x1C */
    __IO uint32_t TRISE;       /* 0x20 */
    __IO uint32_t FLTR;        /* 0x24 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)0x40005400u)

#define I2C_CR1_PE          (1u << 0)
#define I2C_CR1_START       (1u << 8)
#define I2C_CR1_STOP        (1u << 9)
#define I2C_CR1_ACK         (1u << 10)
#define I2C_CR1_POS         (1u << 11)
#define I2C_CR1_SWRST       (1u << 15)
#define I2C_SR1_SB          (1u << 0)
#define I2C_SR1_ADDR        (1u << 1)
#define I2C_SR1_BTF         (1u << 2)
#define I2C_SR1_RXNE        (1u << 6)
#define I2C_SR1_TXE         (1u << 7)
#define I2C_SR1_BERR        (1u << 8)
#define I2C_SR1_ARLO        (1u << 9)
#define I2C_SR1_AF          (1u << 10)
#define I2C_SR2_BUSY        (1u << 1)
#define I2C_CCR_FS          (1u << 15)

/* ---- SPI (SPI1 0x4001 3000) ---- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t SR;          /* 0x08 */
    __IO uint32_t DR;          /* 0x0C */
    __IO uint32_t CRCPR;       /* 0x10 */
    __IO uint32_t RXCRCR;      /* 0x14 */
    __IO uint32_t TXCRCR;      /* 0x18 */
    __IO uint32_t I2SCFGR;     /* 0x1C */
    __IO uint32_t I2SPR;       /* 0x20 */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)0x40013000u)

#define SPI_CR1_CPHA        (1u << 0)
#define SPI_CR1_CPOL        (1u << 1)
#define SPI_CR1_MSTR        (1u << 2)
#define SPI_CR1_BR_Pos      3              /* fPCLK / 2^(BR+1) */
#define SPI_CR1_SPE         (1u << 6)
#define SPI_CR1_LSBFIRST    (1u << 7)
#define SPI_CR1_SSI         (1u << 8)
#define SPI_CR1_SSM         (1u << 9)
#define SPI_CR1_DFF         (1u << 11)
#define SPI_CR2_RXDMAEN     (1u << 0)
#define SPI_CR2_TXDMAEN     (1u << 1)
#define SPI_SR_RXNE         (1u << 0)
#define SPI_SR_TXE          (1u << 1)
#define SPI_SR_MODF         (1u << 5)
#define SPI_SR_OVR          (1u << 6)
#define SPI_SR_BSY          (1u << 7)

/* ---- RTC (0x4000 2800) ---- */
typedef struct {
    __IO uint32_t TR;          /* 0x00 시간 */
    __IO uint32_t DR;          /* 0x04 날짜 */
    __IO uint32_t CR;          /* 0x08 */
    __IO uint32_t ISR;         /* 0x0C */
    __IO uint32_t PRER;        /* 0x10 분주 */
    __IO uint32_t WUTR;        /* 0x14 웨이크업 타이머 */
    __IO uint32_t CALIBR;      /* 0x18 */
    __IO uint32_t ALRMAR;      /* 0x1C */
    __IO uint32_t ALRMBR;      /* 0x20 */
    __IO uint32_t WPR;         /* 0x24 쓰기 보호 해제 키 */
} RTC_TypeDef;

#define RTC ((RTC_TypeDef *)0x40002800u)

#define RTC_CR_WUCKSEL_Msk   (7u << 0)
#define RTC_CR_WUCKSEL_1HZ   (4u << 0)      /* 10x: ck_spre (보통 1 Hz) */
#define RTC_CR_WUTE          (1u << 10)
#define RTC_CR_WUTIE         (1u << 14)
#define RTC_ISR_WUTWF        (1u << 2)
#define RTC_ISR_INIT         (1u << 7)
#define RTC_ISR_WUTF         (1u << 10)

#define EXTI_LINE_RTC_WKUP   22u
#define RTC_WKUP_IRQn        3

/* ---- SCB 시스템 제어 레지스터 (0xE000 ED10) ---- */
#define SCB_VTOR            (*(volatile uint32_t *)0xE000ED08u)   /* 벡터 테이블 위치 (32장) */
#define SCB_AIRCR           (*(volatile uint32_t *)0xE000ED0Cu)
#define SCB_AIRCR_SYSRESET  ((0x5FAu << 16) | (1u << 2))             /* VECTKEY + SYSRESETREQ */
#define SCB_SCR             (*(volatile uint32_t *)0xE000ED10u)
#define SCB_CCR             (*(volatile uint32_t *)0xE000ED14u)
#define SCB_SHCSR           (*(volatile uint32_t *)0xE000ED24u)
#define SCB_CFSR            (*(volatile uint32_t *)0xE000ED28u)   /* MMFSR[7:0], BFSR[15:8], UFSR[31:16] */
#define SCB_HFSR            (*(volatile uint32_t *)0xE000ED2Cu)
#define SCB_MMFAR           (*(volatile uint32_t *)0xE000ED34u)
#define SCB_BFAR            (*(volatile uint32_t *)0xE000ED38u)
#define SCB_CCR_UNALIGN_TRP (1u << 3)
#define SCB_CCR_DIV_0_TRP   (1u << 4)
#define SCB_SHCSR_MEMFAULTENA (1u << 16)
#define SCB_SHCSR_BUSFAULTENA (1u << 17)
#define SCB_SHCSR_USGFAULTENA (1u << 18)
#define SCB_SCR_SLEEPONEXIT (1u << 1)
#define SCB_SCR_SLEEPDEEP   (1u << 2)

/* ---- MPU (0xE000 ED90): PM0214 4.2절. 25장 ---- */
#define MPU_TYPE            (*(volatile uint32_t *)0xE000ED90u)
#define MPU_CTRL            (*(volatile uint32_t *)0xE000ED94u)
#define MPU_RNR             (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR            (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR            (*(volatile uint32_t *)0xE000EDA0u)
#define MPU_CTRL_ENABLE     (1u << 0)
#define MPU_CTRL_HFNMIENA   (1u << 1)
#define MPU_CTRL_PRIVDEFENA (1u << 2)   /* 영역 밖은 기본 메모리 맵으로 (특권 모드) */
#define MPU_RASR_ENABLE     (1u << 0)
#define MPU_RASR_SIZE_Pos   1u          /* 영역 크기 = 2^(SIZE+1) 바이트 */
#define MPU_RASR_AP_Pos     24u         /* 000 = 접근 금지 */
#define MPU_RASR_XN         (1u << 28)  /* 실행 금지 */
#define MPU_RASR_S          (1u << 18)
#define MPU_RASR_C          (1u << 17)

static inline void wfi(void) { __asm volatile ("wfi" ::: "memory"); }

/* ---- ADC1 (0x4001 2000), 공통 레지스터 (0x4001 2300) ---- */
typedef struct {
    __IO uint32_t SR;          /* 0x00 */
    __IO uint32_t CR1;         /* 0x04 */
    __IO uint32_t CR2;         /* 0x08 */
    __IO uint32_t SMPR1;       /* 0x0C 채널 10~18 샘플링 시간 */
    __IO uint32_t SMPR2;       /* 0x10 채널 0~9 샘플링 시간 */
    __IO uint32_t JOFR[4];     /* 0x14 ~ 0x20 */
    __IO uint32_t HTR;         /* 0x24 */
    __IO uint32_t LTR;         /* 0x28 */
    __IO uint32_t SQR1;        /* 0x2C 변환 개수 L[23:20] */
    __IO uint32_t SQR2;        /* 0x30 */
    __IO uint32_t SQR3;        /* 0x34 첫 번째 변환 채널 SQ1[4:0] */
    __IO uint32_t JSQR;        /* 0x38 */
    __IO uint32_t JDR[4];      /* 0x3C ~ 0x48 */
    __IO uint32_t DR;          /* 0x4C */
} ADC_TypeDef;

typedef struct {
    __IO uint32_t CSR;         /* 0x00 */
    __IO uint32_t CCR;         /* 0x04 */
    __IO uint32_t CDR;         /* 0x08 */
} ADC_Common_TypeDef;

#define ADC1        ((ADC_TypeDef *)0x40012000u)
#define ADC_COMMON  ((ADC_Common_TypeDef *)0x40012300u)

#define ADC_SR_EOC          (1u << 1)
#define ADC_SR_OVR          (1u << 5)
#define ADC_CR1_RES_Msk     (3u << 24)      /* 00 = 12비트 */
#define ADC_CR2_ADON        (1u << 0)
#define ADC_CR2_CONT        (1u << 1)
#define ADC_CR2_DMA         (1u << 8)
#define ADC_CR2_DDS         (1u << 9)
#define ADC_CR2_EOCS        (1u << 10)
#define ADC_CR2_ALIGN       (1u << 11)
#define ADC_CR2_EXTSEL_Msk  (0xFu << 24)
#define ADC_CR2_EXTSEL_TIM3_TRGO (8u << 24)
#define ADC_CR2_EXTEN_RISING (1u << 28)
#define ADC_CR2_EXTEN_Msk   (3u << 28)
#define ADC_CR2_SWSTART     (1u << 30)
#define ADC_CCR_ADCPRE_Msk  (3u << 16)      /* 00 = PCLK2 ÷2 */
#define ADC_CCR_VBATE       (1u << 22)
#define ADC_CCR_TSVREFE     (1u << 23)

/* 샘플링 시간 코드 (3비트): 0=3, 1=15, 2=28, 3=56, 4=84, 5=112, 6=144, 7=480 클럭 */
#define ADC_SMP_480         7u

#define ADC_CH_TEMP         18u             /* 온도 센서 (VBAT와 같은 채널, VBATE = 0이어야 함) */
#define ADC_CH_VREFINT      17u

/* 공장 보정값 (시스템 메모리, 읽기 전용) */
#define TS_CAL1      (*(const volatile uint16_t *)0x1FFF7A2Cu)   /* 30 °C, VDDA = 3.3 V */
#define TS_CAL2      (*(const volatile uint16_t *)0x1FFF7A2Eu)   /* 110 °C, VDDA = 3.3 V */
#define VREFINT_CAL  (*(const volatile uint16_t *)0x1FFF7A2Au)   /* 30 °C, VDDA = 3.3 V */

/* ---- 인터럽트 번호 (RM0383 10.1.3절 표, 7장에서 쓰는 것만) ---- */
#define EXTI0_IRQn       6
#define ADC_IRQn        18
#define TIM2_IRQn       28
#define TIM3_IRQn       29
#define TIM4_IRQn       30
#define USART2_IRQn     38
#define EXTI15_10_IRQn  40
#define DMA2_Stream0_IRQn 56
#define OTG_FS_IRQn     67

/* ---- Cortex-M4 코어 주변장치 ---- */
typedef struct {
    __IO uint32_t ISER[8];     /* 0xE000E100 인터럽트 허용 (1을 쓰면 허용) */
    uint32_t      RESERVED0[24];
    __IO uint32_t ICER[8];     /* 0xE000E180 인터럽트 금지 (1을 쓰면 금지) */
    uint32_t      RESERVED1[24];
    __IO uint32_t ISPR[8];     /* 0xE000E200 대기 상태로 만들기 */
    uint32_t      RESERVED2[24];
    __IO uint32_t ICPR[8];     /* 0xE000E280 대기 상태 지우기 */
    uint32_t      RESERVED3[24];
    __IO uint32_t IABR[8];     /* 0xE000E300 실행 중 표시 (읽기 전용) */
    uint32_t      RESERVED4[56];
    __IO uint8_t  IP[240];     /* 0xE000E400 우선순위, IRQ 하나에 1바이트 */
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)0xE000E100u)

/* STM32F4는 우선순위 바이트의 상위 4비트만 쓴다: 0(가장 높음) ~ 15(가장 낮음) */
#define NVIC_PRIO_BITS 4

static inline void nvic_enable_irq(uint32_t irqn)
{
    NVIC->ISER[irqn >> 5] = 1u << (irqn & 31u);
}

static inline void nvic_disable_irq(uint32_t irqn)
{
    NVIC->ICER[irqn >> 5] = 1u << (irqn & 31u);
}

static inline void nvic_set_priority(uint32_t irqn, uint32_t prio)
{
    NVIC->IP[irqn] = (uint8_t)((prio << (8 - NVIC_PRIO_BITS)) & 0xFFu);
}

/* SysTick (0xE000 E010): 8장 */
typedef struct {
    __IO uint32_t CTRL;        /* 0x00 */
    __IO uint32_t LOAD;        /* 0x04 */
    __IO uint32_t VAL;         /* 0x08 */
    __IO uint32_t CALIB;       /* 0x0C */
} SysTick_TypeDef;

#define SYSTICK ((SysTick_TypeDef *)0xE000E010u)

#define SYSTICK_CTRL_ENABLE    (1u << 0)
#define SYSTICK_CTRL_TICKINT   (1u << 1)
#define SYSTICK_CTRL_CLKSOURCE (1u << 2)    /* 1 = 프로세서 클럭(HCLK) */
#define SYSTICK_CTRL_COUNTFLAG (1u << 16)

/* 전체 인터럽트 켜고 끄기 (PRIMASK) */
static inline void irq_disable(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void irq_enable(void)  { __asm volatile ("cpsie i" ::: "memory"); }

static inline uint32_t irq_save(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}

static inline void irq_restore(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

#endif /* STM32F411_REGS_H */
