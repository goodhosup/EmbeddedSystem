/*
 * 27장 실습: 태스크 간 동기화와 통신 (NUCLEO-F411RE, FreeRTOS)
 *
 * 태스크                우선순위  쓰는 도구
 *   button                 4      태스크 알림 ← EXTI ISR (지연 처리)
 *   console                2      큐 ← UART 수신 ISR, 명령 처리
 *   producer (100 ms)      2      큐로 측정값을 보냄
 *   consumer               1      큐에서 측정값을 받음
 *   (실험용 태스크는 명령마다 만들고 끝나면 스스로 지운다)
 *
 * 모든 출력은 log_printf()로: print_lock 뮤텍스가 한 줄씩 지켜 준다.
 *
 * 명령 (115200 bps)
 *   r  경쟁 조건: 보호 없이 / 뮤텍스로 공유 카운터 증가
 *   i  우선순위 역전: 이진 세마포어 / 뮤텍스(우선순위 상속)로 high의 대기 시간 비교
 *   s  큐와 태스크 상태
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#include "stm32f411_regs.h"
#include "uart_rtos.h"

#define BUTTON_PIN 13u

static SemaphoreHandle_t print_lock;
static QueueHandle_t     sample_q;
static EventGroupHandle_t ready_bits;
static TaskHandle_t      button_handle;

#define READY_PRODUCER (1u << 0)
#define READY_CONSUMER (1u << 1)

typedef struct {
    uint32_t seq;
    uint32_t tick;
    int32_t  value;
} sample_t;

static volatile uint32_t produced, consumed, dropped;

/* ---- 공유 자원: UART 출력 ---- */
static void log_printf(const char *fmt, ...)
{
    va_list ap;
    xSemaphoreTake(print_lock, portMAX_DELAY);     /* 한 번에 한 태스크만 printf */
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    xSemaphoreGive(print_lock);
}

/* ---- 바쁘게 일하기: 실제 CPU 시간 ms만큼 (선점당한 시간은 세지 않는다) ---- */
static uint32_t loops_per_ms;

static void burn_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t n = 0; n < loops_per_ms; n++) {
        }
    }
}

static void calibrate(void)
{
    uint32_t t0 = DWT_CYCCNT;
    for (volatile uint32_t n = 0; n < 10000u; n++) {
    }
    uint32_t cycles = DWT_CYCCNT - t0;
    loops_per_ms = (uint32_t)(10000ull * 16000u / cycles);    /* 1 ms = 16000 사이클 */
}

/* ---- ISR → 태스크: 버튼 ---- */
void EXTI15_10_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    if (EXTI->PR & (1u << BUTTON_PIN)) {
        EXTI->PR = 1u << BUTTON_PIN;
        vTaskNotifyGiveFromISR(button_handle, &woken);   /* ISR은 알리기만 한다 */
    }
    portYIELD_FROM_ISR(woken);
}

static void button_task(void *arg)
{
    (void)arg;
    uint32_t presses = 0;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);           /* ISR이 알릴 때까지 Blocked */
        vTaskDelay(pdMS_TO_TICKS(30));                     /* 바운싱이 가라앉기를 기다린다 */
        ulTaskNotifyTake(pdTRUE, 0);                       /* 그 사이 쌓인 알림은 버린다 */
        if ((GPIOC->IDR & (1u << BUTTON_PIN)) == 0) {      /* 아직 눌려 있으면 진짜 눌림 */
            log_printf("button pressed (%lu)\n", (unsigned long)++presses);
        }
    }
}

/* ---- 생산자–소비자 ---- */
static void producer_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    uint32_t seq = 0;
    xEventGroupSetBits(ready_bits, READY_PRODUCER);
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(100));
        sample_t s = { seq++, (uint32_t)xTaskGetTickCount(), (int32_t)(DWT_CYCCNT & 0xFFu) };
        if (xQueueSend(sample_q, &s, 0) == pdPASS) {       /* 큐가 가득 차면 기다리지 않고 버린다 */
            produced++;
        } else {
            dropped++;
        }
    }
}

static void consumer_task(void *arg)
{
    (void)arg;
    sample_t s;
    xEventGroupSetBits(ready_bits, READY_CONSUMER);
    for (;;) {
        if (xQueueReceive(sample_q, &s, portMAX_DELAY) == pdPASS) {   /* 값이 올 때까지 Blocked */
            consumed++;
            (void)s;               /* 여기서 필터링, 기록, 전송 등을 한다 */
        }
    }
}

/* ---- 실험 1: 경쟁 조건 ---- */
#define RACE_N 1000u
static volatile uint32_t shared_counter;
static SemaphoreHandle_t counter_lock;
static SemaphoreHandle_t done_sem;

static void race_task(void *arg)
{
    bool use_lock = (arg != NULL);
    for (uint32_t i = 0; i < RACE_N; i++) {
        if (use_lock) {
            xSemaphoreTake(counter_lock, portMAX_DELAY);
        }
        uint32_t v = shared_counter;       /* 읽기 */
        taskYIELD();                       /* 하필 여기서 다른 태스크로 넘어간다면? */
        shared_counter = v + 1u;           /* 쓰기 */
        if (use_lock) {
            xSemaphoreGive(counter_lock);
        }
    }
    xSemaphoreGive(done_sem);
    vTaskDelete(NULL);
}

static void run_race(bool use_lock)
{
    shared_counter = 0;
    void *arg = use_lock ? (void *)1 : NULL;
    xTaskCreate(race_task, "raceA", 256, arg, 1, NULL);
    xTaskCreate(race_task, "raceB", 256, arg, 1, NULL);
    xSemaphoreTake(done_sem, portMAX_DELAY);
    xSemaphoreTake(done_sem, portMAX_DELAY);
    log_printf("%-8s expected %lu, got %lu\n", use_lock ? "mutex" : "no lock",
               (unsigned long)(2u * RACE_N), (unsigned long)shared_counter);
}

/* ---- 실험 2: 우선순위 역전 ---- */
static SemaphoreHandle_t res_lock;           /* 이진 세마포어 또는 뮤텍스 */
static SemaphoreHandle_t low_has_lock;
static volatile uint32_t high_wait_us;

static void low_task(void *arg)              /* 우선순위 1 */
{
    (void)arg;
    xSemaphoreTake(res_lock, portMAX_DELAY);
    xSemaphoreGive(low_has_lock);            /* console에게 "자원을 잡았다" (console이 바로 선점) */
    burn_ms(20);                             /* 자원을 쥐고 20 ms 일한다 */
    xSemaphoreGive(res_lock);
    vTaskDelete(NULL);
}

static void med_task(void *arg)              /* 우선순위 3: 자원과 무관하게 CPU만 50 ms 쓴다 */
{
    (void)arg;
    burn_ms(50);
    vTaskDelete(NULL);
}

static void high_task(void *arg)             /* 우선순위 4 */
{
    (void)arg;
    uint32_t t0 = DWT_CYCCNT;
    xSemaphoreTake(res_lock, portMAX_DELAY); /* low가 쥐고 있으므로 Blocked */
    high_wait_us = (DWT_CYCCNT - t0) / 16u;
    xSemaphoreGive(res_lock);
    xSemaphoreGive(done_sem);
    vTaskDelete(NULL);
}

static void run_inversion(bool use_mutex)
{
    if (use_mutex) {
        res_lock = xSemaphoreCreateMutex();          /* 우선순위 상속 있음 */
    } else {
        res_lock = xSemaphoreCreateBinary();         /* 우선순위 상속 없음 */
        xSemaphoreGive(res_lock);
    }
    xTaskCreate(low_task, "low", 256, NULL, 1, NULL);
    xSemaphoreTake(low_has_lock, portMAX_DELAY);     /* low가 자원을 잡을 때까지 */
    xTaskCreate(high_task, "high", 256, NULL, 4, NULL);   /* 바로 실행되어 자원을 기다리며 Blocked */
    xTaskCreate(med_task, "med", 256, NULL, 3, NULL);     /* low보다 높고 high보다 낮다 */
    xSemaphoreTake(done_sem, portMAX_DELAY);         /* high가 끝날 때까지 */
    vTaskDelay(pdMS_TO_TICKS(60));                   /* med까지 끝나고 Idle이 지운 태스크를 정리하도록 */
    log_printf("%-16s high waited %lu us\n", use_mutex ? "mutex:" : "binary semaphore:",
               (unsigned long)high_wait_us);
    vSemaphoreDelete(res_lock);
}

/* ---- 콘솔 ---- */
static void console_task(void *arg)
{
    (void)arg;
    char c;
    xEventGroupWaitBits(ready_bits, READY_PRODUCER | READY_CONSUMER,
                        pdFALSE, pdTRUE, portMAX_DELAY);   /* 두 태스크가 모두 준비될 때까지 */
    log_printf("\nch27 sync: r=race i=inversion s=status, B1=button\n");
    for (;;) {
        xQueueReceive(uart_rx_queue(), &c, portMAX_DELAY);  /* 글자가 올 때까지 Blocked */
        switch (c) {
        case 'r':
            run_race(false);
            run_race(true);
            break;
        case 'i':
            run_inversion(false);
            run_inversion(true);
            break;
        case 's':
            log_printf("samples: produced %lu, consumed %lu, dropped %lu, waiting %lu\n",
                       (unsigned long)produced, (unsigned long)consumed, (unsigned long)dropped,
                       (unsigned long)uxQueueMessagesWaiting(sample_q));
            log_printf("heap free %u B, console stack free %lu B\n", (unsigned)xPortGetFreeHeapSize(),
                       (unsigned long)uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
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

static void button_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    GPIOC->MODER &= ~(3u << (BUTTON_PIN * 2u));
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFu << 4)) | (2u << 4);
    EXTI->FTSR |=  (1u << BUTTON_PIN);
    EXTI->RTSR &= ~(1u << BUTTON_PIN);
    EXTI->PR    =  (1u << BUTTON_PIN);
    EXTI->IMR  |=  (1u << BUTTON_PIN);
    nvic_set_priority(EXTI15_10_IRQn, 6);            /* FromISR API를 부르므로 5~15 */
    nvic_enable_irq(EXTI15_10_IRQn);
}

int main(void)
{
    cycle_counter_init();
    calibrate();

    print_lock   = xSemaphoreCreateMutex();
    counter_lock = xSemaphoreCreateMutex();
    done_sem     = xSemaphoreCreateCounting(4, 0);
    low_has_lock = xSemaphoreCreateBinary();
    sample_q     = xQueueCreate(8, sizeof(sample_t));   /* 값을 복사해서 넣는다 */
    ready_bits   = xEventGroupCreate();

    uart_rtos_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);

    xTaskCreate(button_task,   "button",   256, NULL, 4, &button_handle);
    xTaskCreate(console_task,  "console",  512, NULL, 2, NULL);
    xTaskCreate(producer_task, "producer", 256, NULL, 2, NULL);
    xTaskCreate(consumer_task, "consumer", 256, NULL, 1, NULL);
    button_init();                                   /* 알릴 태스크를 만든 뒤에 인터럽트를 켠다 */

    vTaskStartScheduler();
    for (;;) {
    }
}
