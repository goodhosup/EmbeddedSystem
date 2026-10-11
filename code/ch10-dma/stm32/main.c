/*
 * 10장 실습: DMA (NUCLEO-F411RE)
 *
 * 1) 메모리 → 메모리 복사: CPU 루프와 DMA2 스트림 1의 걸린 클럭 수 비교
 *    (DWT 사이클 카운터로 잰다. 결과: cpu_copy_cycles, dma_copy_cycles)
 *
 * 2) TIM3(1 kHz) → ADC1(PA0) → DMA2 스트림 0(원형) → adc_buf[64]
 *    하프 전송(HT)과 전송 완료(TC) 인터럽트에서 절반씩 평균을 내서
 *    LD2(PA5, TIM2 PWM) 밝기를 정한다. 메인 루프는 아무것도 하지 않는다.
 *
 * 연결: PA0(A0)에 9장의 가변저항
 *
 * 레지스터: RM0383 9장 DMA (9.3.3절 요청 매핑 표, 9.5절 레지스터), 11장 ADC
 */
#include "stm32f411_regs.h"

#define COPY_WORDS  1024u
#define ADC_BUF_LEN 64u

static uint32_t src[COPY_WORDS];
static uint32_t dst[COPY_WORDS];

volatile uint32_t cpu_copy_cycles, dma_copy_cycles;
volatile uint32_t copy_ok;

static volatile uint16_t adc_buf[ADC_BUF_LEN];
volatile uint32_t half_count, full_count;
volatile uint16_t pot_avg;

/* ------------------------------------------------------------------ */
/* 1) 메모리 → 메모리                                                    */
/* ------------------------------------------------------------------ */

static void copy_benchmark(void)
{
    for (uint32_t i = 0; i < COPY_WORDS; i++) {
        src[i] = i * 2654435761u;           /* 아무 값이나 채운다 */
    }

    /* CPU 복사 */
    uint32_t t0 = DWT_CYCCNT;
    for (uint32_t i = 0; i < COPY_WORDS; i++) {
        dst[i] = src[i];
    }
    cpu_copy_cycles = DWT_CYCCNT - t0;

    for (uint32_t i = 0; i < COPY_WORDS; i++) {
        dst[i] = 0;
    }

    /* DMA 복사: 메모리 → 메모리는 DMA2만 할 수 있다. PAR이 원본, M0AR이 대상 */
    DMA_Stream_TypeDef *st = &DMA2->S[1];
    st->CR &= ~DMA_SxCR_EN;
    while (st->CR & DMA_SxCR_EN) {          /* 스트림이 실제로 꺼질 때까지 */
    }
    DMA2->LIFCR = DMA_ALL_FLAGS0 << dma_flag_shift(1);
    st->PAR  = (uint32_t)src;
    st->M0AR = (uint32_t)dst;
    st->NDTR = COPY_WORDS;
    st->FCR  = (1u << 2) | 3u;              /* DMDIS = 1(FIFO 사용, M2M에 필요), FTH = 꽉 참 */
    st->CR   = DMA_SxCR_DIR_M2M | DMA_SxCR_PINC | DMA_SxCR_MINC
             | DMA_SxCR_PSIZE_32 | DMA_SxCR_MSIZE_32 | DMA_SxCR_PL_HIGH;

    t0 = DWT_CYCCNT;
    st->CR |= DMA_SxCR_EN;
    while ((DMA2->LISR & (DMA_TCIF0 << dma_flag_shift(1))) == 0) {
        /* 기다리는 동안 CPU는 다른 일을 할 수 있다. 여기서는 측정을 위해 기다린다 */
    }
    dma_copy_cycles = DWT_CYCCNT - t0;
    DMA2->LIFCR = DMA_TCIF0 << dma_flag_shift(1);

    copy_ok = 1;
    for (uint32_t i = 0; i < COPY_WORDS; i++) {
        if (dst[i] != src[i]) {
            copy_ok = 0;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 2) TIM3 → ADC1 → DMA2 스트림 0 (원형, HT/TC)                          */
/* ------------------------------------------------------------------ */

static void process(const volatile uint16_t *p, uint32_t n)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < n; i++) {
        sum += p[i];
    }
    pot_avg = (uint16_t)(sum / n);
    TIM2->CCR1 = pot_avg;
}

void DMA2_Stream0_IRQHandler(void)
{
    uint32_t isr = DMA2->LISR;              /* 스트림 0의 플래그는 비트 0~5 */

    if (isr & DMA_HTIF0) {
        DMA2->LIFCR = DMA_HTIF0;
        half_count++;
        process(&adc_buf[0], ADC_BUF_LEN / 2);              /* 앞쪽 절반 */
    }
    if (isr & DMA_TCIF0) {
        DMA2->LIFCR = DMA_TCIF0;
        full_count++;
        process(&adc_buf[ADC_BUF_LEN / 2], ADC_BUF_LEN / 2); /* 뒤쪽 절반 */
    }
    if (isr & (DMA_TEIF0 | DMA_DMEIF0 | DMA_FEIF0)) {
        DMA2->LIFCR = DMA_TEIF0 | DMA_DMEIF0 | DMA_FEIF0;   /* 오류: 이 예제에서는 지우기만 */
    }
}

static void adc_dma_init(void)
{
    /* PA0 아날로그 */
    GPIOA->MODER |= (3u << (0 * 2));

    /* DMA2 스트림 0, 채널 0 = ADC1 (RM0383 9.3.3절 표) */
    DMA_Stream_TypeDef *st = &DMA2->S[0];
    st->CR &= ~DMA_SxCR_EN;
    while (st->CR & DMA_SxCR_EN) {
    }
    DMA2->LIFCR = DMA_ALL_FLAGS0;
    st->PAR  = (uint32_t)&ADC1->DR;
    st->M0AR = (uint32_t)adc_buf;
    st->NDTR = ADC_BUF_LEN;
    st->FCR  = 0;                                            /* 직접 모드 (FIFO 안 씀) */
    st->CR   = DMA_SxCR_CHSEL(0) | DMA_SxCR_DIR_P2M
             | DMA_SxCR_MINC                                 /* 메모리 주소만 증가 */
             | DMA_SxCR_PSIZE_16 | DMA_SxCR_MSIZE_16
             | DMA_SxCR_CIRC                                 /* 끝나면 처음부터 */
             | DMA_SxCR_HTIE | DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    nvic_set_priority(DMA2_Stream0_IRQn, 5);
    nvic_enable_irq(DMA2_Stream0_IRQn);
    st->CR |= DMA_SxCR_EN;

    /* ADC1: 채널 0, TIM3_TRGO 상승 에지로 시작, 변환마다 DMA 요청 */
    ADC_COMMON->CCR &= ~ADC_CCR_ADCPRE_Msk;                  /* PCLK2 ÷2 = 8 MHz */
    ADC1->CR1  = 0;                                          /* 12비트 */
    ADC1->SMPR2 = (ADC1->SMPR2 & ~7u) | 4u;                  /* 채널 0: 84 클럭 */
    ADC1->SQR1 = 0;                                          /* 1개 */
    ADC1->SQR3 = 0;                                          /* 채널 0 */
    ADC1->CR2  = ADC_CR2_EXTEN_RISING | ADC_CR2_EXTSEL_TIM3_TRGO
               | ADC_CR2_DMA | ADC_CR2_DDS                   /* DDS: 원형 DMA에서 요청을 계속 */
               | ADC_CR2_ADON;

    /* TIM3: 1 kHz 업데이트 → TRGO */
    TIM3->PSC = 15;                                          /* 1 MHz */
    TIM3->ARR = 999;                                         /* 1 kHz */
    TIM3->CR2 = (TIM3->CR2 & ~TIM_CR2_MMS_Msk) | TIM_CR2_MMS_UPDATE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void led_pwm_init(void)
{
    gpio_set_af(GPIOA, 5, 1);
    TIM2->PSC   = 15;
    TIM2->ARR   = 4095;
    TIM2->CCMR1 = (TIM2->CCMR1 & ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_CC1S_Msk))
                | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM2->CCER |= TIM_CCER_CC1E;
    TIM2->EGR   = TIM_EGR_UG;
    TIM2->CR1  |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    cycle_counter_init();
    copy_benchmark();

    led_pwm_init();
    adc_dma_init();

    for (;;) {
        /* 할 일이 없다. ADC 변환, 데이터 이동, 평균 계산은 모두 DMA와 ISR이 한다.
           11장에서는 여기서 CPU를 잠재운다 */
    }
}
