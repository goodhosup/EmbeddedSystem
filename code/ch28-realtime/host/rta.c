#include "rta.h"
#include <math.h>

double rta_utilization(const rt_task_t *tasks, size_t n)
{
    double u = 0.0;
    for (size_t i = 0; i < n; i++) {
        u += (double)tasks[i].C / (double)tasks[i].T;
    }
    return u;
}

double rta_ll_bound(size_t n)
{
    return (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
}

static uint32_t ceil_div(uint32_t a, uint32_t b)
{
    return (a + b - 1u) / b;
}

/*
 * R = C_i + B_i + Σ_{j<i} ceil(R / T_j) · C_j  를 R이 변하지 않을 때까지 반복한다.
 * 높은 우선순위 태스크 j가 R 동안 최대 ceil(R/T_j)번 끼어든다는 뜻이다.
 */
uint32_t rta_response_time(const rt_task_t *tasks, size_t i)
{
    const rt_task_t *t = &tasks[i];
    uint32_t r = t->C + t->B;
    for (;;) {
        uint32_t next = t->C + t->B;
        for (size_t j = 0; j < i; j++) {
            next += ceil_div(r, tasks[j].T) * tasks[j].C;
        }
        if (next > t->D) {
            return UINT32_MAX;          /* 마감을 넘는다: 스케줄 불가능 */
        }
        if (next == r) {
            return r;
        }
        r = next;
    }
}

void rta_sort_rm(rt_task_t *tasks, size_t n)
{
    for (size_t i = 1; i < n; i++) {            /* 태스크 수가 적으므로 삽입 정렬 */
        rt_task_t key = tasks[i];
        size_t j = i;
        while (j > 0 && tasks[j - 1].T > key.T) {
            tasks[j] = tasks[j - 1];
            j--;
        }
        tasks[j] = key;
    }
}
