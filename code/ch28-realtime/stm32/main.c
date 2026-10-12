/*
 * 28장 실습: 인터럽트 지연·지터 측정, 응답 시간 측정, 간이 트레이스 (NUCLEO-F411RE, FreeRTOS)
 *
 * 1) 인터럽트 지연: TIM2가 1 ms마다 갱신 인터럽트를 낸다. 카운터는 그 순간 0부터 다시 세므로,
 *    ISR 첫머리에서 읽은 TIM2->CNT가 곧 "인터럽트 발생 → ISR 시작"까지의 지연(16 MHz 클럭 수)이다.
 * 2) 응답 시간: 주기 태스크 3개(ctrl 5 ms/1 ms, comm 20 ms/4 ms, log 50 ms/10 ms)의
 *    "깨어날 시각 → 일 끝"을 재서 응답 시간 분석의 예측(1, 5, 18 ms, host/rta)과 비교한다.
 * 3) 트레이스: traceTASK_SWITCHED_IN 훅으로 문맥 전환 시각을 링 버퍼에 남긴다.
 *
 * 명령 (115200 bps)
 *   l  인터럽트 지연 통계 (최소/최대, 분포) 후 초기화
 *   c  크리티컬 섹션 태스크 켜기/끄기 (5 ms마다 인터럽트를 100 µs 막는다)
 *   p  TIM2 인터럽트 우선순위 6 ↔ 2 (커널이 막는 범위 안/밖)
 *   r  태스크별 최대 응답 시간과 예측값
 *   t  최근 문맥 전환 32개
 */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stm32f411_regs.h"
#include "uart_rtos.h"

#define CPU_HZ        16000000u
#define CYC_PER_US    16u

/* ---- 바쁘게 일하기: CPU 시간 기준 (27장과 같은 보정 방법) ---- */
static uint32_t loops_per_100us;

static void burn_us(uint32_t us)
{
    for (uint32_t i = 0; i < us / 100u; i++) {
        for (volatile uint32_t n = 0; n < loops_per_100us; n++) {
        }
    }
}

static void calibrate(void)
{
    uint32_t t0 = DWT_CYCCNT;
    for (volatile uint32_t n = 0; n < 10000u; n++) {
    }
    uint32_t cycles = DWT_CYCCNT - t0;
    loops_per_100us = (uint32_t)(10000ull * (100u * CYC_PER_US) / cycles);
}

/* ---- 1) 인터럽트 지연 ---- */
static const uint32_t bucket_us[] = { 1, 2, 5, 10, 20, 50, 100 };   /* 이 값 미만 */
#define N_BUCKETS (sizeof bucket_us / sizeof bucket_us[0] + 1u)
static volatile uint32_t lat_min = UINT32_MAX, lat_max, lat_count;
static volatile uint32_t lat_hist[N_BUCKETS];
static volatile uint32_t tim2_prio = 6;

void TIM2_IRQHandler(void)
{
    uint32_t cnt = TIM2->CNT;                  /* 가장 먼저 읽는다 */
    TIM2->SR = ~TIM_SR_UIF;
    if (cnt < lat_min) lat_min = cnt;
    if (cnt > lat_max) lat_max = cnt;
    uint32_t us = cnt / CYC_PER_US;
    unsigned b = 0;
    while (b < N_BUCKETS - 1u && us >= bucket_us[b]) {
        b++;
    }
    lat_hist[b]++;
    lat_count++;
    /* 이 ISR은 FreeRTOS API를 부르지 않으므로 우선순위 2(커널 범위 밖)로도 둘 수 있다 */
}

static void tim2_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    TIM2->PSC  = 0;                            /* 16 MHz 그대로: 1 카운트 = 62.5 ns */
    TIM2->ARR  = CPU_HZ / 1000u - 1u;          /* 1 ms */
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;
    TIM2->DIER = TIM_DIER_UIE;
    nvic_set_priority(TIM2_IRQn, tim2_prio);
    nvic_enable_irq(TIM2_IRQn);
    TIM2->CR1  = TIM_CR1_CEN;
}

/* ---- 2) 주기 태스크와 응답 시간 ---- */
typedef struct {
    const char *name;
    uint32_t    period_ms;
    uint32_t    wcet_us;
    uint32_t    predicted_ms;      /* host/rta가 계산한 값 */
    volatile uint32_t max_resp_ms;
    volatile uint32_t misses;
} periodic_t;

static periodic_t ptasks[] = {
    { "ctrl",  5,  1000,  1, 0, 0 },
    { "comm", 20,  4000,  5, 0, 0 },
    { "log",  50, 10000, 18, 0, 0 },
};

static void periodic_task(void *arg)
{
    periodic_t *p = (periodic_t *)arg;
    TickType_t release = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&release, pdMS_TO_TICKS(p->period_ms));   /* release = 이번 주기의 시작 시각 */
        burn_us(p->wcet_us);
        uint32_t resp = (uint32_t)(xTaskGetTickCount() - release); /* 틱(1 ms) 단위 */
        if (resp > p->max_resp_ms) {
            p->max_resp_ms = resp;
        }
        if (resp > p->period_ms) {
            p->misses++;
        }
    }
}

/* 크리티컬 섹션으로 인터럽트를 막는 태스크 (지연 실험용) */
static TaskHandle_t crit_handle;

static void crit_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5));
        taskENTER_CRITICAL();                  /* BASEPRI: 우선순위 5~15의 인터럽트를 막는다 */
        burn_us(100);
        taskEXIT_CRITICAL();
    }
}

/* ---- 3) 간이 트레이스 ---- */
#define TRACE_N 32u
typedef struct {
    uint32_t     cycles;
    TaskHandle_t task;
} trace_rec_t;
static trace_rec_t trace_buf[TRACE_N];
static volatile uint32_t trace_head;
static volatile bool trace_on = true;

void trace_switched_in(void)                   /* 커널이 문맥 전환 중에 부른다: 아주 짧게 */
{
    if (trace_on) {
        uint32_t i = trace_head;
        trace_buf[i].cycles = DWT_CYCCNT;
        trace_buf[i].task = xTaskGetCurrentTaskHandle();
        trace_head = (i + 1u) & (TRACE_N - 1u);
    }
}

static void dump_trace(void)
{
    trace_on = false;                          /* 출력하는 동안 기록을 멈춘다 */
    uint32_t start = trace_head;
    uint32_t t0 = trace_buf[start].cycles;
    printf("    time(us)  task\n");
    for (uint32_t k = 0; k < TRACE_N; k++) {
        const trace_rec_t *r = &trace_buf[(start + k) & (TRACE_N - 1u)];
        if (r->task != NULL) {
            printf("%12lu  %s\n", (unsigned long)((r->cycles - t0) / CYC_PER_US), pcTaskGetName(r->task));
        }
    }
    trace_on = true;
}

/* ---- 콘솔 ---- */
static void print_latency(void)
{
    uint32_t mn = lat_min, mx = lat_max, n = lat_count;
    printf("TIM2 prio %lu, %lu interrupts: latency min %lu cyc (%lu ns), max %lu cyc (%lu ns)\n",
           (unsigned long)tim2_prio, (unsigned long)n, (unsigned long)mn, (unsigned long)(mn * 1000u / CYC_PER_US),
           (unsigned long)mx, (unsigned long)(mx * 1000u / CYC_PER_US));
    for (unsigned b = 0; b < N_BUCKETS; b++) {
        if (b < N_BUCKETS - 1u) {
            printf("  < %3lu us : %lu\n", (unsigned long)bucket_us[b], (unsigned long)lat_hist[b]);
        } else {
            printf("  >=%3lu us : %lu\n", (unsigned long)bucket_us[b - 1u], (unsigned long)lat_hist[b]);
        }
        lat_hist[b] = 0;
    }
    lat_min = UINT32_MAX;
    lat_max = 0;
    lat_count = 0;
}

static void console_task(void *arg)
{
    (void)arg;
    bool crit_on = false;
    char c;
    printf("\nch28 real-time: l=latency c=critical p=prio r=response t=trace\n");
    for (;;) {
        xQueueReceive(uart_rx_queue(), &c, portMAX_DELAY);
        switch (c) {
        case 'l':
            print_latency();
            break;
        case 'c':
            crit_on = !crit_on;
            if (crit_on) vTaskResume(crit_handle); else vTaskSuspend(crit_handle);
            printf("critical-section task %s\n", crit_on ? "on" : "off");
            break;
        case 'p':
            tim2_prio = (tim2_prio == 6u) ? 2u : 6u;
            nvic_set_priority(TIM2_IRQn, tim2_prio);
            printf("TIM2 priority %lu (%s)\n", (unsigned long)tim2_prio,
                   tim2_prio < 5u ? "above configMAX_SYSCALL: never masked by the kernel" : "masked in critical sections");
            break;
        case 'r':
            printf("%-5s %6s %10s %10s %7s\n", "task", "T(ms)", "max R(ms)", "RTA R(ms)", "misses");
            for (unsigned i = 0; i < 3; i++) {
                printf("%-5s %6lu %10lu %10lu %7lu\n", ptasks[i].name, (unsigned long)ptasks[i].period_ms,
                       (unsigned long)ptasks[i].max_resp_ms, (unsigned long)ptasks[i].predicted_ms,
                       (unsigned long)ptasks[i].misses);
            }
            break;
        case 't':
            dump_trace();
            break;
        default:
            break;
        }
    }
}

/* ---- 훅 ---- */
static void panic_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

void vApplicationIdleHook(void)
{
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** stack overflow: ");
    panic_puts(name);
    panic_puts(" ***\r\n");
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** heap exhausted ***\r\n");
    for (;;) {
    }
}

void freertos_assert_failed(const char *file, int line)
{
    (void)line;
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** configASSERT: ");
    panic_puts(file);
    panic_puts(" ***\r\n");
    __asm volatile ("bkpt #0");
    for (;;) {
    }
}

int main(void)
{
    cycle_counter_init();
    calibrate();
    uart_rtos_init(115200, CPU_HZ);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 비율 단조: 주기가 짧을수록 높은 우선순위. console은 가장 낮게 (출력이 측정을 흐트러뜨리지 않게) */
    xTaskCreate(periodic_task, "ctrl", 256, &ptasks[0], 4, NULL);
    xTaskCreate(periodic_task, "comm", 256, &ptasks[1], 3, NULL);
    xTaskCreate(periodic_task, "log",  256, &ptasks[2], 2, NULL);
    xTaskCreate(crit_task,     "crit", 256, NULL,       1, &crit_handle);
    xTaskCreate(console_task,  "console", 512, NULL,    1, NULL);
    vTaskSuspend(crit_handle);

    tim2_init();
    vTaskStartScheduler();
    for (;;) {
    }
}
