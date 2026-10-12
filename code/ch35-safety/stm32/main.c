/*
 * 35장 실습: 워치독과 태스크 감시, 리셋 원인, 시작 시 RAM 시험 (NUCLEO-F411RE)
 *
 *   - 시작하면 리셋 원인(RCC_CSR)을 출력하고 플래그를 지운다
 *   - 시험용 RAM 4 KB를 March C-로 시험한다 (고장이면 안전 상태로)
 *   - 세 태스크(control 10 ms, sensor 100 ms, comm 500 ms)가 감시자에게 체크인하고,
 *     모두 기한 안일 때만 독립 워치독(약 1초)을 갱신한다
 * 명령: h = control 태스크를 멈춘다 (다른 태스크는 계속 돈다) → 감시자가 알아채고 워치독 리셋
 *       w = 메인 루프 전체를 멈춘다 → 워치독 리셋
 */
#include <stdbool.h>
#include <stdio.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "supervisor.h"
#include "march.h"

static volatile uint32_t ms;
void SysTick_Handler(void) { ms++; }

/* ---- 리셋 원인 ----
 * 플래그는 여러 개가 함께 선다: 내부 리셋(워치독, 소프트웨어, 저전력)도 NRST 핀을 당기므로 PINRSTF가 같이 서고,
 * 전원 투입 때는 PORRSTF와 함께 BORRSTF, PINRSTF도 선다. 그래서 우선순위로 하나만 고른다. */
static const char *reset_cause(uint32_t csr)
{
    if (csr & RCC_CSR_PORRSTF)  return "power-on";
    if (csr & RCC_CSR_IWDGRSTF) return "INDEPENDENT-WATCHDOG";
    if (csr & RCC_CSR_WWDGRSTF) return "window-watchdog";
    if (csr & RCC_CSR_LPWRRSTF) return "low-power";
    if (csr & RCC_CSR_SFTRSTF)  return "software";
    if (csr & RCC_CSR_BORRSTF)  return "brown-out";
    if (csr & RCC_CSR_PINRSTF)  return "reset-pin";       /* 다른 원인이 없을 때만 */
    return "unknown";
}

static void report_reset_cause(void)
{
    printf("\nreset cause: %s\n", reset_cause(RCC->CSR));
    RCC->CSR |= RCC_CSR_RMVF;           /* 플래그를 지워 두어야 다음 리셋의 원인을 구별할 수 있다 */
}

/* ---- 시작 시 RAM 시험: 전용 4 KB 구역 ---- */
static uint32_t test_area[1024];
static uint32_t rd(void *ctx, size_t i) { return ((volatile uint32_t *)ctx)[i]; }
static void wr(void *ctx, size_t i, uint32_t v) { ((volatile uint32_t *)ctx)[i] = v; }

static void safe_state(const char *why)
{
    __asm volatile ("cpsid i");
    printf("SAFE STATE: %s\n", why);    /* 실제 제품: 출력 끄기, 경보, 기록 */
    for (;;) {
        IWDG->KR = IWDG_KEY_RELOAD;     /* 안전 상태를 유지하며 머문다 (리셋 반복을 막는 설계 선택) */
    }
}

static void iwdg_start_1s(void)
{
    IWDG->KR  = IWDG_KEY_START;
    IWDG->KR  = IWDG_KEY_ACCESS;
    IWDG->PR  = 3u;                     /* LSI 약 32 kHz / 32 = 1 kHz */
    IWDG->RLR = 1000u;                  /* 약 1초 (LSI 오차로 0.7~1.9초) */
    while (IWDG->SR != 0) {
    }
    IWDG->KR  = IWDG_KEY_RELOAD;
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    report_reset_cause();

    mem_if_t m = { rd, wr, test_area };
    long bad = march_c_minus(&m, sizeof test_area / sizeof test_area[0]);
    if (bad >= 0) {
        safe_state("RAM test failed");
    }
    printf("RAM test (March C-, 4 KB): ok\n");

    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    supervisor_t sup;
    sup_init(&sup);
    int t_ctrl = sup_register(&sup, "control", 30, ms);
    int t_sens = sup_register(&sup, "sensor", 300, ms);
    int t_comm = sup_register(&sup, "comm", 1500, ms);
    iwdg_start_1s();
    printf("watchdog on. h = hang control task, w = hang everything\n");

    uint32_t next_ctrl = 0, next_sens = 0, next_comm = 0;
    bool ctrl_hung = false;
    bool reported = false;
    for (;;) {
        uint32_t now = ms;
        if (!ctrl_hung && (int32_t)(now - next_ctrl) >= 0) { next_ctrl = now + 10;  sup_checkin(&sup, t_ctrl, now); }
        if ((int32_t)(now - next_sens) >= 0)               { next_sens = now + 100; sup_checkin(&sup, t_sens, now); }
        if ((int32_t)(now - next_comm) >= 0)               { next_comm = now + 500; sup_checkin(&sup, t_comm, now); }

        if (sup_check(&sup, now)) {
            IWDG->KR = IWDG_KEY_RELOAD;  /* 모두 살아 있을 때만 */
        } else if (!reported) {
            printf("task '%s' missed its deadline: stop feeding the watchdog\n", sup.e[sup.failed].name);
            reported = true;             /* 실제 제품: 여기서 비휘발성 메모리에 원인을 기록 */
        }

        int c = uart2_getc();
        if (c == 'h') { ctrl_hung = true; printf("control task hung\n"); }
        if (c == 'w') { printf("main loop hung\n"); for (;;) { } }
    }
}
