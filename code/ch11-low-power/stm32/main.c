/*
 * 11장 실습: Stop 모드와 RTC 웨이크업 타이머, 버튼 깨우기 (NUCLEO-F411RE)
 *
 * 동작
 *   - RTC 웨이크업 타이머(LSI 기반, 약 2초)로 깨어나 LD2를 잠깐 켜고 다시 Stop.
 *   - B1(PC13)을 누르면 EXTI로 즉시 깨어나 LD2를 길게 켠다.
 *
 * 전류 측정: 보드의 JP6(IDD) 점퍼를 빼고 멀티미터(µA 범위)를 연결한다.
 * 주의: Stop 모드 중에는 디버거 연결이 끊긴다. 디버깅할 때는 DBGMCU 설정이 필요하다(23장).
 *
 * 레지스터: RM0383 5.3절(저전력 모드), 17장(RTC), 10.3절(EXTI 라인 22)
 */
#include "stm32f411_regs.h"

static volatile uint32_t wake_by_button;
volatile uint32_t wake_count;

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & (1u << 13)) {
        EXTI->PR = 1u << 13;
        wake_by_button = 1;
    }
}

void RTC_WKUP_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_WUTF) {
        /* WUTF는 0을 써서 지운다(rc_w0). &= 로 쓰면 읽은 뒤 쓰기 전에 세트된 다른 플래그
           (ALRAF 등)까지 0으로 지울 수 있으므로, 지울 비트에만 0을 쓰고 나머지는 1을 쓴다.
           INIT(쓰기 가능한 제어 비트)은 현재 값을 보존한다.
           ISR[13:8]의 플래그는 쓰기 보호(WPR)를 풀지 않아도 지울 수 있다. */
        RTC->ISR = ~(RTC_ISR_WUTF | RTC_ISR_INIT) | (RTC->ISR & RTC_ISR_INIT);
    }
    EXTI->PR = 1u << EXTI_LINE_RTC_WKUP;        /* EXTI 라인 22도 지운다 */
}

static void delay(volatile uint32_t n)
{
    while (n--) {
    }
}

static void rtc_wakeup_init(uint32_t seconds)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_DBP;                      /* 백업 영역 쓰기 허용 */

    RCC->CSR |= RCC_CSR_LSION;                  /* LSI(약 32 kHz) 켜기 */
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0) {
    }
    RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_Msk) | RCC_BDCR_RTCSEL_LSI | RCC_BDCR_RTCEN;

    RTC->WPR = 0xCA;                            /* 쓰기 보호 해제 키 */
    RTC->WPR = 0x53;

    RTC->CR &= ~RTC_CR_WUTE;                    /* 웨이크업 타이머 끄고 */
    while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {   /* 설정 가능해질 때까지 */
    }
    /* PRER 기본값(비동기 127, 동기 255): 32768 Hz → 1 Hz. LSI는 약 32 kHz라 약간 다르다 */
    RTC->WUTR = seconds - 1u;
    RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL_Msk) | RTC_CR_WUCKSEL_1HZ | RTC_CR_WUTIE | RTC_CR_WUTE;

    RTC->WPR = 0xFF;                            /* 다시 잠그기 */

    EXTI->IMR  |= 1u << EXTI_LINE_RTC_WKUP;     /* RTC 웨이크업은 EXTI 라인 22 상승 에지 */
    EXTI->RTSR |= 1u << EXTI_LINE_RTC_WKUP;
    nvic_set_priority(RTC_WKUP_IRQn, 6);
    nvic_enable_irq(RTC_WKUP_IRQn);
}

static void button_exti_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    GPIOC->MODER &= ~(3u << (13 * 2));
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFu << 4)) | (2u << 4);
    EXTI->FTSR |= 1u << 13;
    EXTI->IMR  |= 1u << 13;
    nvic_set_priority(EXTI15_10_IRQn, 5);
    nvic_enable_irq(EXTI15_10_IRQn);
}

static void enter_stop(void)
{
    /* Stop 모드: 딥 슬립 + PDDS = 0, 저전력 레귤레이터, Flash 절전 */
    PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS | PWR_CR_FPDS | PWR_CR_CWUF;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    wfi();                                      /* 여기서 멈추고, 깨어나면 ISR 실행 후 이어서 */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    /* 깨어나면 시스템 클럭은 HSI 16 MHz다. PLL을 쓰고 있었다면 여기서 다시 설정한다(3장) */
}

static void unused_pins_analog(void)
{
    /* 쓰지 않는 핀을 아날로그 모드로: 디지털 입력이 꺼져 새는 전류가 가장 적다.
       PA13, PA14(SWD)는 남겨 둔다 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    uint32_t keep_swd = GPIOA->MODER & (0xFu << 26);     /* PA13, PA14 */
    GPIOA->MODER = keep_swd | 0x03FFFFFFu | (3u << 30);  /* PA0~12, PA15: 아날로그 */
    GPIOA->MODER = (GPIOA->MODER & ~(3u << (5 * 2))) | (1u << (5 * 2));   /* PA5: LED 출력 */
    GPIOB->MODER = 0xFFFFFFFFu;
    GPIOC->MODER = 0xFFFFFFFFu & ~(3u << (13 * 2));   /* PC13은 버튼 입력 */
}

int main(void)
{
    unused_pins_analog();
    button_exti_init();
    rtc_wakeup_init(2);

    for (;;) {
        wake_count++;
        GPIOA->BSRR = 1u << 5;
        delay(wake_by_button ? 400000u : 20000u);
        wake_by_button = 0;
        GPIOA->BSRR = 1u << (5 + 16);

        enter_stop();
    }
}
