/*
 * 24장 실습: 이벤트 기반 세탁기 (NUCLEO-F411RE)
 *
 * 계층
 *   app/washer.c      상태 머신. 하드웨어를 모른다
 *   service/evq.c     이벤트 큐 (ISR → 메인)
 *   service/sched.c   협력형 스케줄러, 소프트웨어 타이머
 *   stm32/main.c      보드 계층: 핀, 인터럽트, 그리고 washer_io_t 구현
 *
 * 입력
 *   B1 (PC13) 또는 UART 's'  시작/일시정지
 *   UART 'o'                 문 열기 (DONE에서만 의미 있음)
 *   UART 't'                 태스크별 최대 실행 시간 출력
 * 출력
 *   LD2 (PA5)  IDLE 꺼짐, 동작 중 빠른 깜박임, PAUSED 느린 깜박임, DONE 켜짐
 *   UART2 115200 bps: 상태 전이와 액추에이터 동작 로그
 *
 * 실제 밸브, 펌프, 모터 대신 plant_task()가 물높이를 흉내 내어
 * 수위 센서 이벤트(EV_FULL, EV_EMPTY)를 만든다.
 */
#include "stm32f411_regs.h"
#include "uart2.h"
#include "port.h"
#include "washer.h"
#include "evq.h"
#include "sched.h"

#define SYSCLK_HZ   16000000u
#define LED_PIN     5u
#define BUTTON_PIN  13u
#define DEBOUNCE_MS 50u

static volatile uint32_t ms_ticks;
static volatile uint32_t button_ms;     /* 마지막으로 받아들인 버튼 시각 */
static evq_t    events;
static washer_t washer;
static swtimer_t washer_timer;

/* 흉내 낸 기계 상태 (보드 계층만 안다) */
static bool    valve_on, pump_on;
static uint8_t motor_mode;
static int     water_level;             /* 0 ~ 100 % */

/* ---- 출력 도우미 ---- */
static void puts_uart(const char *s)
{
    while (*s) {
        uart2_putc(*s++);
    }
}

static void put_u32(uint32_t v)
{
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    do {
        buf[--i] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    puts_uart(&buf[i]);
}

static void log_msg(const char *a, const char *b)
{
    put_u32(ms_ticks);
    puts_uart(" ms  ");
    puts_uart(a);
    puts_uart(b);
    puts_uart("\r\n");
}

/* ---- 인터럽트 ---- */
void SysTick_Handler(void)
{
    ms_ticks++;
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & (1u << BUTTON_PIN)) {
        EXTI->PR = 1u << BUTTON_PIN;
        uint32_t now = ms_ticks;
        if (now - button_ms >= DEBOUNCE_MS) {       /* 바운싱으로 생긴 에지는 버린다 */
            button_ms = now;
            evq_put(&events, EV_START);              /* ISR은 이벤트만 넣고 끝 */
        }
    }
}

/* ---- washer_io_t 구현: 앱이 요청한 동작을 보드에서 실행 ---- */
static void io_valve(bool on)     { valve_on = on;  log_msg("valve ", on ? "on" : "off"); }
static void io_pump(bool on)      { pump_on = on;   log_msg("pump ", on ? "on" : "off"); }
static void io_motor(uint8_t m)   { motor_mode = m; log_msg("motor ", m == MOTOR_SPIN ? "spin" : m == MOTOR_WASH ? "wash" : "stop"); }
static void io_door_lock(bool on) { log_msg("door ", on ? "locked" : "unlocked"); }
static void io_timer_start(uint32_t ms) { swtimer_start(&washer_timer, ms_ticks, ms); }
static void io_timer_stop(void)   { swtimer_stop(&washer_timer); }

static const washer_io_t board_io = {
    .valve       = io_valve,
    .pump        = io_pump,
    .motor       = io_motor,
    .door_lock   = io_door_lock,
    .timer_start = io_timer_start,
    .timer_stop  = io_timer_stop,
};

/* ---- 태스크: 모두 짧게 일하고 돌아온다 ---- */
static void plant_task(void);
static void timer_task(void);
static void uart_task(void);
static void led_task(void);

static task_t tasks[] = {
    { "plant", plant_task, 100, 0, 0 },
    { "timer", timer_task,  10, 0, 0 },
    { "uart ", uart_task,   10, 0, 0 },
    { "led  ", led_task,   100, 0, 0 },
};

#define N_TASKS (sizeof tasks / sizeof tasks[0])

static void plant_task(void)              /* 100 ms: 물높이 흉내 */
{
    if (valve_on && water_level < 100) {
        water_level += 10;
        if (water_level >= 100) {
            water_level = 100;
            evq_put(&events, EV_FULL);
        }
    }
    if (pump_on && water_level > 0) {
        water_level -= 20;
        if (water_level <= 0) {
            water_level = 0;
            evq_put(&events, EV_EMPTY);
        }
    }
}

static void timer_task(void)              /* 10 ms: 소프트웨어 타이머 */
{
    if (swtimer_expired(&washer_timer, ms_ticks)) {
        evq_put(&events, EV_TIMEOUT);
    }
}

static void uart_task(void)               /* 10 ms: 명령 받기 */
{
    int c = uart2_getc();
    if (c == 's') {
        evq_put(&events, EV_START);
    } else if (c == 'o') {
        evq_put(&events, EV_DOOR_OPEN);
    } else if (c == 't') {
        for (unsigned i = 0; i < N_TASKS; i++) {
            puts_uart(tasks[i].name);
            puts_uart(" max ");
            put_u32(tasks[i].max_us);
            puts_uart(" us\r\n");
        }
    }
}

static void led_task(void)                /* 100 ms: 상태 표시 */
{
    static uint32_t n;
    bool on;
    n++;
    switch (washer.state) {
    case WS_IDLE:   on = false;          break;
    case WS_DONE:   on = true;           break;
    case WS_PAUSED: on = (n / 5u) & 1u;  break;   /* 0.5초마다 */
    default:        on = n & 1u;         break;   /* 0.1초마다 */
    }
    GPIOA->BSRR = on ? (1u << LED_PIN) : (1u << (LED_PIN + 16u));
}

/* ---- 보드 초기화 ---- */
static void board_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    GPIOA->MODER = (GPIOA->MODER & ~(3u << (LED_PIN * 2u))) | (1u << (LED_PIN * 2u));

    /* B1: PC13 입력 (보드에 외부 풀업), EXTI 라인 13 하강 에지 (7장) */
    GPIOC->MODER &= ~(3u << (BUTTON_PIN * 2u));
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFu << 4)) | (2u << 4);
    EXTI->FTSR |=  (1u << BUTTON_PIN);
    EXTI->RTSR &= ~(1u << BUTTON_PIN);
    EXTI->PR    =  (1u << BUTTON_PIN);
    EXTI->IMR  |=  (1u << BUTTON_PIN);
    nvic_set_priority(EXTI15_10_IRQn, 5);
    nvic_enable_irq(EXTI15_10_IRQn);

    SYSTICK->LOAD = SYSCLK_HZ / 1000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    cycle_counter_init();
    uart2_init(115200u, SYSCLK_HZ);
}

int main(void)
{
    board_init();
    evq_init(&events);
    puts_uart("\r\n24장 세탁기: B1 또는 's' 시작/일시정지, 'o' 문 열기, 't' 태스크 시간\r\n");
    washer_init(&washer, &board_io);

    for (;;) {
        sched_run_due(tasks, N_TASKS, ms_ticks);

        int ev;
        while ((ev = evq_get(&events)) >= 0) {     /* 쌓인 이벤트를 모두 처리 */
            washer_state_t before = washer.state;
            washer_dispatch(&washer, (event_t)ev);
            if (washer.state != before) {
                log_msg("state ", washer_state_name(washer.state));
            }
        }

        wfi();     /* 다음 인터럽트(최소 1 ms 뒤 SysTick)까지 잠든다 (11장) */
    }
}
