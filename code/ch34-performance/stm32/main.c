/*
 * 34장 실습: 측정하고 최적화하기 (NUCLEO-F411RE)
 *
 * 같은 FIR 필터(256 표본, 32 탭)를 세 가지로 구현해 DWT 사이클 카운터로 잰다.
 *   1) 16 MHz HSI, Flash 대기 0
 *   2) 100 MHz PLL, Flash 대기 3, ART(프리페치 + 명령/데이터 캐시) 켬
 *   3) 100 MHz PLL, Flash 대기 3, ART 끔
 * 최적화 수준은 빌드할 때 고른다: make OPT=-O0 / -Og / -O2 / -O3 / -Os
 * 결과는 USART2 (16 MHz에서 115200, 100 MHz에서는 APB1 50 MHz에 맞춰 다시 설정)
 */
#include <stdio.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "dsp.h"

#define N    256
#define TAPS 32

static float xf[N + TAPS - 1], hf[TAPS], yf[N];
static q15_t xq[N + TAPS - 1], hq[TAPS], yq[N];

static void make_data(void)
{
    for (int k = 0; k < TAPS; k++) {
        hf[k] = 0.03f;
        hq[k] = q15_from_float(hf[k]);
    }
    uint32_t seed = 12345;
    for (int i = 0; i < N + TAPS - 1; i++) {
        seed = seed * 1103515245u + 12345u;                /* 간단한 의사 난수 */
        xq[i] = (q15_t)(seed >> 17);
        xf[i] = q15_to_float(xq[i]);
    }
}

/* 함수 하나를 몇 번 실행해 가장 짧은 사이클을 고른다 (인터럽트 등 잡음 제거) */
typedef void (*bench_fn)(void);
static void run_f32(void)  { fir_f32(xf, hf, yf, N, TAPS); }
static void run_ref(void)  { fir_q15_ref(xq, hq, yq, N, TAPS); }
static void run_fast(void) { fir_q15_fast(xq, hq, yq, N, TAPS); }

static uint32_t measure(bench_fn f)
{
    uint32_t best = UINT32_MAX;
    for (int r = 0; r < 5; r++) {
        uint32_t t0 = DWT_CYCCNT;
        f();
        uint32_t c = DWT_CYCCNT - t0;
        if (c < best) best = c;
    }
    return best;
}

static void report(const char *title, uint32_t mhz)
{
    static const struct { const char *name; bench_fn f; } b[] = {
        { "fir_f32     ", run_f32 }, { "fir_q15_ref ", run_ref }, { "fir_q15_fast", run_fast },
    };
    printf("\n%s\n", title);
    for (unsigned i = 0; i < 3; i++) {
        uint32_t c = measure(b[i].f);
        printf("  %s %7lu cycles  %6lu us  %5lu.%02lu cycles/tap\n", b[i].name, (unsigned long)c,
               (unsigned long)(c / mhz), (unsigned long)(c / (N * TAPS)),
               (unsigned long)((c * 100u / (N * TAPS)) % 100u));
    }
}

/* 3장의 100 MHz 설정 (HSI → PLL) */
static void clock_100mhz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | PWR_CR_VOS_SCALE1;
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | 3u | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk)) | RCC_CFGR_PPRE1_DIV2;
    uint32_t pll = RCC->PLLCFGR;
    pll &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLSRC_HSE | RCC_PLLCFGR_PLLQ_Msk);
    pll |= (8u << RCC_PLLCFGR_PLLM_Pos) | (100u << RCC_PLLCFGR_PLLN_Pos) | (0u << RCC_PLLCFGR_PLLP_Pos) | (4u << RCC_PLLCFGR_PLLQ_Pos);
    RCC->PLLCFGR = pll;
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    cycle_counter_init();
    make_data();
    printf("\nch34 FIR benchmark: %d samples x %d taps (build: %s)\n", N, TAPS, BUILD_OPT);

    report("16 MHz HSI, 0 wait states", 16);

    clock_100mhz();
    uart2_init(115200, 50000000u);                         /* APB1 = 50 MHz */
    report("100 MHz PLL, 3 wait states, ART on", 100);

    FLASH->ACR &= ~(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    report("100 MHz PLL, 3 wait states, ART off", 100);

    for (;;) {
        __asm volatile ("wfi");
    }
}
