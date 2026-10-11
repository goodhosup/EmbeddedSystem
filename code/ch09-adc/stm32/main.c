/*
 * 9장 실습: 가변저항으로 LED 밝기 조절, 내부 온도 센서와 VDDA 측정 (NUCLEO-F411RE)
 *
 * 연결
 *   PA0 (A0, ADC1_IN0): 10 kΩ 가변저항의 가운데 다리. 양쪽 다리는 3.3 V와 GND
 *   PA5 (LD2): 8장의 TIM2_CH1 PWM으로 밝기 조절
 *
 * GDB에서 확인: print pot_avg, print temp_c_x100, print vdda_mv
 *
 * 레지스터: RM0383 11.12절 ADC registers
 * 클럭: HSI 16 MHz, APB2 ÷1 → PCLK2 16 MHz, ADCPRE ÷2 → ADCCLK 8 MHz
 */
#include "stm32f411_regs.h"

#define AVG_N 8

static volatile uint32_t ms_ticks;
volatile uint16_t pot_raw, pot_avg;
volatile int32_t  temp_c_x100;          /* 온도 × 100 (예: 2534 = 25.34 °C) */
volatile uint32_t vdda_mv;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void adc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    GPIOA->MODER |= (3u << (0 * 2));                 /* PA0: 아날로그 모드(11) */

    ADC_COMMON->CCR = (ADC_COMMON->CCR & ~(ADC_CCR_ADCPRE_Msk | ADC_CCR_VBATE))
                    | ADC_CCR_TSVREFE;               /* ÷2, 온도 센서와 VREFINT 켜기 */
    ADC1->CR1 &= ~ADC_CR1_RES_Msk;                   /* 12비트 */
    ADC1->CR2  = 0;                                  /* 단일 변환, 오른쪽 정렬 */

    /* 샘플링 시간: 모든 채널 480 클럭 (8 MHz에서 60 µs). 온도 센서는 최소 10 µs 필요 */
    ADC1->SMPR2 = 0x3FFFFFFFu;                       /* 채널 0~9: 3비트 × 10 = 모두 111 */
    ADC1->SMPR1 = 0x07FFFFFFu;                       /* 채널 10~18: 3비트 × 9 = 모두 111 */

    ADC1->SQR1 = 0;                                  /* 변환 개수 L = 0 → 1개 */
    ADC1->CR2 |= ADC_CR2_ADON;                       /* ADC 켜기 */
    for (volatile int i = 0; i < 100; i++) {         /* 켜진 뒤 안정화 시간(tSTAB, 수 µs) */
    }
}

static uint16_t adc_read(uint32_t ch)
{
    ADC1->SQR3 = ch;                                 /* 첫 번째(유일한) 변환 = ch */
    ADC1->CR2 |= ADC_CR2_SWSTART;                    /* 변환 시작 */
    while ((ADC1->SR & ADC_SR_EOC) == 0) {           /* 변환 끝 기다리기 */
    }
    return (uint16_t)ADC1->DR;                       /* DR을 읽으면 EOC가 지워진다 */
}

static void led_pwm_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOA, 5, 1);
    TIM2->PSC   = 15;
    TIM2->ARR   = 4095;                              /* ADC 값(0~4095)을 그대로 CCR에 쓸 수 있게 */
    TIM2->CCMR1 = (TIM2->CCMR1 & ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_CC1S_Msk))
                | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM2->CCER |= TIM_CCER_CC1E;
    TIM2->EGR   = TIM_EGR_UG;
    TIM2->CR1  |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

static void measure_internal(void)
{
    uint16_t vref = adc_read(ADC_CH_VREFINT);
    uint16_t ts   = adc_read(ADC_CH_TEMP);

    /* VDDA = 3.3 V × VREFINT_CAL ÷ 지금 읽은 VREFINT */
    vdda_mv = 3300u * VREFINT_CAL / vref;

    /* 온도 센서 값을 3.3 V 기준으로 환산한 뒤, 30 °C와 110 °C 보정점 사이를 직선으로 보간 */
    int32_t ts_3v3 = (int32_t)ts * (int32_t)vdda_mv / 3300;
    temp_c_x100 = 3000 + (ts_3v3 - (int32_t)TS_CAL1) * (11000 - 3000)
                       / ((int32_t)TS_CAL2 - (int32_t)TS_CAL1);
}

int main(void)
{
    static uint16_t buf[AVG_N];
    uint32_t idx = 0, sum = 0;

    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    adc_init();
    led_pwm_init();

    uint32_t last = 0, last_temp = 0;
    for (;;) {
        uint32_t now = ms_ticks;

        if (now - last >= 10u) {
            last = now;
            pot_raw = adc_read(0);
            sum -= buf[idx];
            buf[idx] = pot_raw;
            sum += pot_raw;
            idx = (idx + 1u) % AVG_N;
            pot_avg = (uint16_t)(sum / AVG_N);
            TIM2->CCR1 = pot_avg;
        }

        if (now - last_temp >= 500u) {
            last_temp = now;
            measure_internal();
        }
    }
}
