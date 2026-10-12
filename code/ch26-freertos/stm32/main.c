/*
 * 26장 실습: FreeRTOS 태스크, 우선순위, 선점 (NUCLEO-F411RE)
 *
 * 태스크                우선순위  하는 일
 *   sample   (주기 10 ms)   3      vTaskDelayUntil로 주기 실행, 주기 흔들림(지터)을 잰다
 *   console  (20 ms 폴링)   2      UART 명령 처리, 상태 출력 (printf는 이 태스크만 쓴다)
 *   blink    (500 ms)       1      LD2 깜박임
 *   hog                      1      CPU를 놓지 않는 계산. 'h'로 켜고 끈다
 *   (Idle)                   0      커널이 만든다. 할 일이 없을 때 실행
 *
 * 명령 (115200 bps)
 *   l  태스크 목록: 상태, 우선순위, 남은 스택
 *   h  hog 태스크 켜기/끄기
 *   j  sample 태스크의 주기 지터 (최소/최대 µs)
 *   o  스택이 작은 태스크를 만들어 일부러 넘치게 한다 → 스택 오버플로 훅
 */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f411_regs.h"
#include "uart2.h"

#define LED_PIN 5u

static TaskHandle_t hog_handle;
static volatile uint32_t idle_count;

/* sample 태스크가 재는 값 (console이 읽기만 한다. 32비트 읽기는 원자적) */
static volatile uint32_t period_min_us = UINT32_MAX, period_max_us, samples;

/* ---- 태스크 ---- */
static void sample_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t prev = DWT_CYCCNT;
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));   /* 정확히 10 ms마다 깨어난다 */
        uint32_t now = DWT_CYCCNT;
        uint32_t us = (now - prev) / 16u;                  /* 빼기 먼저 (24장) */
        prev = now;
        if (samples++ > 0) {                               /* 첫 번째는 기준점 */
            if (us < period_min_us) period_min_us = us;
            if (us > period_max_us) period_max_us = us;
        }
        /* 여기서 센서를 읽고 필터를 돌린다고 생각하자 */
    }
}

static void blink_task(void *arg)
{
    (void)arg;
    for (;;) {
        GPIOA->ODR ^= 1u << LED_PIN;
        vTaskDelay(pdMS_TO_TICKS(500));                    /* 기다리는 동안 CPU를 다른 태스크에 */
    }
}

static void hog_task(void *arg)
{
    (void)arg;
    volatile uint32_t x = 0;
    for (;;) {
        x++;                                               /* 절대 블록하지 않는다 */
    }
}

static void tiny_task(void *arg)
{
    (void)arg;
    volatile uint8_t big[512];                             /* 스택 128워드(512 B)에 다 안 들어간다 */
    for (unsigned i = 0; i < sizeof big; i++) {
        big[i] = (uint8_t)i;
    }
    for (;;) {
        vTaskDelay(1);                                     /* 문맥 전환 때 커널이 넘침을 발견한다 */
    }
}

static const char *state_name(eTaskState s)
{
    switch (s) {
    case eRunning:   return "Running";
    case eReady:     return "Ready";
    case eBlocked:   return "Blocked";
    case eSuspended: return "Suspended";
    case eDeleted:   return "Deleted";
    default:         return "?";
    }
}

static void list_tasks(void)
{
    static TaskStatus_t st[8];
    UBaseType_t n = uxTaskGetSystemState(st, 8, NULL);
    printf("%-10s %-10s %4s %10s\n", "task", "state", "prio", "free stack");
    for (UBaseType_t i = 0; i < n; i++) {
        printf("%-10s %-10s %4lu %6lu B\n", st[i].pcTaskName, state_name(st[i].eCurrentState),
               (unsigned long)st[i].uxCurrentPriority,
               (unsigned long)st[i].usStackHighWaterMark * sizeof(StackType_t));
    }
    printf("heap free %u B (min ever %u B), idle loops %lu\n", (unsigned)xPortGetFreeHeapSize(),
           (unsigned)xPortGetMinimumEverFreeHeapSize(), (unsigned long)idle_count);
}

static void console_task(void *arg)
{
    (void)arg;
    bool hog_on = false;
    printf("\nch26 FreeRTOS: l=list h=hog j=jitter o=overflow\n");
    for (;;) {
        int c = uart2_getc();
        switch (c) {
        case 'l':
            list_tasks();
            break;
        case 'h':
            hog_on = !hog_on;
            if (hog_on) {
                vTaskResume(hog_handle);
            } else {
                vTaskSuspend(hog_handle);
            }
            printf("hog %s\n", hog_on ? "on" : "off");
            break;
        case 'j':
            printf("sample period: min %lu us, max %lu us (%lu samples)\n",
                   (unsigned long)period_min_us, (unsigned long)period_max_us, (unsigned long)samples);
            period_min_us = UINT32_MAX;
            period_max_us = 0;
            break;
        case 'o':
            printf("creating a task with a 512 B stack and a 512 B local array...\n");
            xTaskCreate(tiny_task, "tiny", 128, NULL, 1, NULL);
            break;
        default:
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ---- FreeRTOS 훅 ---- */
void vApplicationIdleHook(void)
{
    idle_count++;            /* 다른 태스크가 모두 Blocked일 때만 여기 온다. 블록하는 함수를 부르면 안 된다 */
}

static void panic_puts(const char *s)
{
    while (*s) {
        uart2_putc(*s++);    /* 커널 상태를 믿을 수 없으므로 printf 대신 폴링 출력 */
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** stack overflow in task: ");
    panic_puts(name);
    panic_puts(" ***\r\n");
    __asm volatile ("bkpt #0");
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** FreeRTOS heap exhausted ***\r\n");
    for (;;) {
    }
}

void freertos_assert_failed(const char *file, int line)
{
    taskDISABLE_INTERRUPTS();
    panic_puts("\r\n*** configASSERT failed: ");
    panic_puts(file);
    panic_puts(" ***\r\n");
    (void)line;              /* 줄 번호는 디버거에서 본다 */
    __asm volatile ("bkpt #0");
    for (;;) {
    }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << (LED_PIN * 2u))) | (1u << (LED_PIN * 2u));
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    cycle_counter_init();

    /* 스택 크기는 워드(4 B) 단위. console은 printf 때문에 넉넉히 */
    xTaskCreate(sample_task,  "sample",  256, NULL, 3, NULL);
    xTaskCreate(console_task, "console", 512, NULL, 2, NULL);
    xTaskCreate(blink_task,   "blink",   128, NULL, 1, NULL);
    xTaskCreate(hog_task,     "hog",     128, NULL, 1, &hog_handle);
    vTaskSuspend(hog_handle);                    /* 처음에는 멈춰 둔다 */

    vTaskStartScheduler();                       /* 돌아오지 않는다 */

    for (;;) {                                   /* 여기 왔다면 Idle 태스크를 만들 힙이 모자란 것 */
    }
}
