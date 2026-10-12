#include "sched.h"
#include "port.h"

void sched_run_due(task_t *tasks, size_t n, uint32_t now_ms)
{
    for (size_t i = 0; i < n; i++) {
        task_t *t = &tasks[i];
        if (!time_reached(now_ms, t->next_ms)) {
            continue;
        }
        /* 늦게 불려도 주기가 밀리지 않도록 now가 아니라 예정 시각에 주기를 더한다 */
        t->next_ms += t->period_ms;
        if (time_reached(now_ms, t->next_ms)) {
            t->next_ms = now_ms + t->period_ms;   /* 한 주기 이상 밀렸으면 따라잡지 않고 다시 맞춘다 */
        }
        uint32_t start = port_cycles();
        t->fn();
        /* 빼기를 먼저: 나눈 값끼리 빼면 넘침 경계가 2^32가 아니게 되어 틀린다 */
        uint32_t used = (port_cycles() - start) / PORT_CYCLES_PER_US;
        if (used > t->max_us) {
            t->max_us = used;
        }
    }
}
