/*
 * 3장 실습: 같은 지연 루프, 다른 클럭 (NUCLEO-F411RE)
 *
 * 1) 리셋 직후의 HSI 16 MHz로 LED(PA5)를 10번 깜빡인다.
 * 2) PLL로 SYSCLK를 100 MHz로 올린다.
 * 3) 같은 지연 루프로 계속 깜빡인다. 깜빡이는 속도가 약 6배 빨라진다.
 *
 * 클럭 설정 순서: RM0383 6장 "Reset and clock control (RCC)",
 * Flash 대기 상태: RM0383 3.4.1절 "Relation between CPU clock frequency
 * and Flash memory read time" 표 (2.7~3.6 V, 90~100 MHz → 3 WS)
 */
#include "stm32f411_regs.h"

static void delay(volatile uint32_t count)
{
    while (count--) {
        /* 아무것도 하지 않고 시간만 보낸다 */
    }
}

static void led_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;          /* GPIOA 클럭 켜기 */
    GPIOA->MODER &= ~(3u << (5 * 2));
    GPIOA->MODER |=  (1u << (5 * 2));             /* PA5: 범용 출력 */
}

static void led_toggle(void)
{
    GPIOA->ODR ^= (1u << 5);
}

/* HSI 16 MHz → PLL → SYSCLK 100 MHz, AHB 100 MHz, APB1 50 MHz, APB2 100 MHz */
static void clock_init_100mhz(void)
{
    /* 1) 전압 스케일 1: 100 MHz로 동작하려면 필요하다 */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | PWR_CR_VOS_SCALE1;

    /* 2) Flash 대기 상태 3, 프리페치와 캐시 켜기 (클럭을 올리기 전에) */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | 3u
               | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* 3) 버스 분주: AHB ÷1, APB1 ÷2 (최대 50 MHz), APB2 ÷1 */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | RCC_CFGR_PPRE1_DIV2;

    /* 4) PLL 설정: 16 MHz ÷ M(8) = 2 MHz, × N(100) = 200 MHz, ÷ P(2) = 100 MHz
     *    PLLP 필드 값 0은 ÷2, PLLSRC 비트 0은 HSI, Q는 USB용(여기서는 미사용) */
    uint32_t pll = RCC->PLLCFGR;        /* 예약 비트는 리셋 값을 유지해야 하므로 읽어서 고친다 */
    pll &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk
             | RCC_PLLCFGR_PLLSRC_HSE | RCC_PLLCFGR_PLLQ_Msk);
    pll |= (8u   << RCC_PLLCFGR_PLLM_Pos)
         | (100u << RCC_PLLCFGR_PLLN_Pos)
         | (0u   << RCC_PLLCFGR_PLLP_Pos)
         | (4u   << RCC_PLLCFGR_PLLQ_Pos);
    RCC->PLLCFGR = pll;

    /* 5) PLL 켜고 안정될 때까지 기다리기 */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
    }

    /* 6) SYSCLK 소스를 PLL로 바꾸고, 실제로 바뀌었는지 확인 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }
}

int main(void)
{
    led_init();

    for (int i = 0; i < 20; i++) {      /* 16 MHz: 10번 깜빡임 (켜기 + 끄기 = 2번 토글) */
        led_toggle();
        delay(1000000);
    }

    clock_init_100mhz();

    for (;;) {                          /* 100 MHz: 같은 루프, 더 빠른 깜빡임 */
        led_toggle();
        delay(1000000);
    }
}
