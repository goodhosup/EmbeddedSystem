/*
 * 8장 실습: SysTick 1 ms 틱, 디바운싱, PWM 밝기, 서보, 입력 캡처 (NUCLEO-F411RE)
 *
 * 클럭: 리셋 기본값 HSI 16 MHz, APB1 ÷1 → 타이머 클럭 16 MHz
 *
 * 연결
 *   PA5  (LD2, TIM2_CH1, AF1): 보드 LED 밝기를 PWM으로 (1 kHz)
 *   PA6  (D12, TIM3_CH1, AF2): 서보 신호선 (50 Hz). 서보 전원은 별도 5 V, GND 공통
 *   PB6  (D10, TIM4_CH1, AF2): 입력 캡처. PA6과 점퍼선으로 연결하면 서보 펄스 폭을 잰다
 *   PC13 (B1): 버튼
 *
 * GDB에서 print pulse_width_us 로 측정값을 확인한다.
 *
 * 레지스터: RM0383 13.4절 (TIM2~TIM5), PM0214 4.5절 (SysTick)
 */
#include "stm32f411_regs.h"

#define TIMER_CLK_HZ 16000000u

static volatile uint32_t ms_ticks;
volatile uint32_t pulse_width_us;          /* 디버거로 볼 수 있게 static을 붙이지 않음 */

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void systick_init(void)
{
    SYSTICK->LOAD = TIMER_CLK_HZ / 1000u - 1u;   /* 16000 클럭 = 1 ms */
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

static void led_pwm_init(void)
{
    /* TIM2 CH1 → PA5, 1 MHz 틱, 1000틱 = 1 kHz */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOA, 5, 1);

    TIM2->PSC   = TIMER_CLK_HZ / 1000000u - 1u;   /* 15 */
    TIM2->ARR   = 1000u - 1u;
    TIM2->CCR1  = 0;
    TIM2->CCMR1 = (TIM2->CCMR1 & ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_CC1S_Msk))
                | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;   /* PWM 모드 1, CCR1 프리로드 */
    TIM2->CCER |= TIM_CCER_CC1E;                     /* 채널 1 출력 켜기 */
    TIM2->CR1  |= TIM_CR1_ARPE;
    TIM2->EGR   = TIM_EGR_UG;                        /* PSC, ARR, CCR1을 즉시 반영 */
    TIM2->CR1  |= TIM_CR1_CEN;
}

static void servo_init(void)
{
    /* TIM3 CH1 → PA6, 1 MHz 틱, 20000틱 = 20 ms */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOA, 6, 2);

    TIM3->PSC   = TIMER_CLK_HZ / 1000000u - 1u;
    TIM3->ARR   = 20000u - 1u;
    TIM3->CCR1  = 1500u;                             /* 1.5 ms */
    TIM3->CCMR1 = (TIM3->CCMR1 & ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_CC1S_Msk))
                | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CR1  |= TIM_CR1_ARPE;
    TIM3->EGR   = TIM_EGR_UG;
    TIM3->CR1  |= TIM_CR1_CEN;
}

static void capture_init(void)
{
    /* TIM4 CH1 ← PB6, 1 MHz 틱, 16비트 자유 실행, 양쪽 에지 캡처 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOB, 6, 2);

    TIM4->PSC   = TIMER_CLK_HZ / 1000000u - 1u;
    TIM4->ARR   = 0xFFFFu;
    TIM4->CCMR1 = (TIM4->CCMR1 & ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1F_Msk))
                | TIM_CCMR1_CC1S_TI1;                /* CC1 = 입력, TI1에서 */
    TIM4->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1NP     /* 11 = 양쪽 에지 */
                | TIM_CCER_CC1E;                     /* 캡처 켜기 */
    TIM4->DIER |= TIM_DIER_CC1IE;
    TIM4->EGR   = TIM_EGR_UG;
    TIM4->CR1  |= TIM_CR1_CEN;

    nvic_set_priority(TIM4_IRQn, 4);
    nvic_enable_irq(TIM4_IRQn);
}

void TIM4_IRQHandler(void)
{
    static uint16_t rise;

    if (TIM4->SR & TIM_SR_CC1IF) {
        uint16_t ccr = (uint16_t)TIM4->CCR1;         /* CCR1을 읽으면 CC1IF가 지워진다 */
        if (GPIOB->IDR & (1u << 6)) {                /* 지금 High → 방금 상승 에지였다 */
            rise = ccr;
        } else {                                     /* 하강 에지 */
            pulse_width_us = (uint16_t)(ccr - rise); /* 16비트 뺄셈: 카운터가 넘쳐도 맞다 */
        }
    }
}

static uint32_t millis(void)
{
    return ms_ticks;            /* 32비트 정렬 읽기는 원자적 */
}

static int button_pressed_event(void)
{
    static uint32_t stable = 1, count = 0;
    uint32_t now = (GPIOC->IDR >> 13) & 1u;

    if (now != stable) {
        if (++count >= 4) {
            stable = now;
            count = 0;
            return stable == 0;
        }
    } else {
        count = 0;
    }
    return 0;
}

int main(void)
{
    static const uint16_t led_steps[]   = { 0, 10, 50, 200, 999 };
    static const uint16_t servo_steps[] = { 1000, 1250, 1500, 1750, 2000 };
    uint32_t step = 0;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    GPIOC->MODER &= ~(3u << (13 * 2));
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3u << (13 * 2))) | (1u << (13 * 2));

    systick_init();
    led_pwm_init();
    servo_init();
    capture_init();

    uint32_t last_sample = 0;

    for (;;) {
        uint32_t now = millis();

        if (now - last_sample >= 5u) {
            last_sample = now;
            if (button_pressed_event()) {
                step = (step + 1u) % (sizeof(led_steps) / sizeof(led_steps[0]));
                TIM2->CCR1 = led_steps[step];
                TIM3->CCR1 = servo_steps[step];
            }
        }
    }
}
