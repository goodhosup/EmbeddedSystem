/*
 * 18장: USB를 위한 클럭 설정 (NUCLEO-F411RE)
 *
 * USB Full Speed는 48 MHz 클럭이 ±0.25% 안에 있어야 한다. 내부 RC(HSI)는 ±1% 수준이라 쓸 수 없다.
 * NUCLEO는 ST-LINK가 8 MHz 클럭을 HSE 입력(바이패스 모드)으로 공급한다(UM1724, 3장).
 *
 *   HSE 8 MHz ÷ M(4) = 2 MHz → × N(192) = 384 MHz (VCO)
 *   ÷ P(4) = 96 MHz (SYSCLK),  ÷ Q(8) = 48 MHz (USB)
 *   AHB 96 MHz, APB1 ÷2 = 48 MHz, APB2 96 MHz, Flash 3 WS (RM0383 3.4.1절)
 */
#include "stm32f411_regs.h"

uint32_t SystemCoreClock = 16000000u;   /* TinyUSB의 dwc2 드라이버가 이 값을 읽는다 */

void clock_init_96mhz_hse(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | PWR_CR_VOS_SCALE1;

    RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;      /* 외부 클럭 입력(크리스털이 아님) */
    while ((RCC->CR & RCC_CR_HSERDY) == 0) {
    }

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | 3u
               | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | RCC_CFGR_PPRE1_DIV2;

    uint32_t pll = RCC->PLLCFGR;
    pll &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk
             | RCC_PLLCFGR_PLLSRC_HSE | RCC_PLLCFGR_PLLQ_Msk);
    pll |= (4u   << RCC_PLLCFGR_PLLM_Pos)
         | (192u << RCC_PLLCFGR_PLLN_Pos)
         | (1u   << RCC_PLLCFGR_PLLP_Pos)       /* 01 = ÷4 */
         | RCC_PLLCFGR_PLLSRC_HSE
         | (8u   << RCC_PLLCFGR_PLLQ_Pos);
    RCC->PLLCFGR = pll;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }
    SystemCoreClock = 96000000u;
}
