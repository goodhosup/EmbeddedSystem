/*
 * STM32F411 레지스터 정의 (이 책의 예제에서 쓰는 것만)
 *
 * 주소와 오프셋의 출처
 *   - 주변장치 시작 주소: RM0383 2.3절 "Memory map" 표
 *   - RCC 레지스터: RM0383 6.3절 "RCC registers"
 *   - GPIO 레지스터: RM0383 8.4절 "GPIO registers"
 *   - FLASH_ACR: RM0383 3.8절 "Flash interface registers"
 *   - PWR_CR: RM0383 5.4절 "PWR registers"
 *   [절 번호는 RM0383 Rev 3 기준, 확인 필요]
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

/* RCC_APB1ENR */
#define RCC_APB1ENR_PWREN   (1u << 28)

/* RCC_CSR: 리셋 원인 플래그 */
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

/* ---- FLASH 인터페이스 (0x4002 3C00) ---- */
typedef struct {
    __IO uint32_t ACR;         /* 0x00 */
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)0x40023C00u)

#define FLASH_ACR_LATENCY_Msk (0xFu << 0)
#define FLASH_ACR_PRFTEN      (1u << 8)
#define FLASH_ACR_ICEN        (1u << 9)
#define FLASH_ACR_DCEN        (1u << 10)

/* ---- PWR (0x4000 7000) ---- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t CSR;         /* 0x04 */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)0x40007000u)

#define PWR_CR_VOS_Msk      (3u << 14)
#define PWR_CR_VOS_SCALE1   (3u << 14)

#endif /* STM32F411_REGS_H */
