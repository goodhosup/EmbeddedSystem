/*
 * 28장: 고정 우선순위 스케줄 가능성 분석
 *   - 이용률 상한 (Liu & Layland, 비율 단조 스케줄링)
 *   - 응답 시간 분석 (Response Time Analysis)
 * 시간 단위는 정수(예: µs 또는 ms)로 통일한다.
 */
#ifndef RTA_H
#define RTA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *name;
    uint32_t    C;      /* 최악 실행 시간 (WCET) */
    uint32_t    T;      /* 주기 */
    uint32_t    D;      /* 마감 시간 (보통 T와 같다) */
    uint32_t    B;      /* 낮은 태스크 때문에 막힐 수 있는 최대 시간 (뮤텍스, 크리티컬 섹션) */
} rt_task_t;

/* tasks[]는 우선순위가 높은 것부터 정렬되어 있어야 한다 */
double   rta_utilization(const rt_task_t *tasks, size_t n);
double   rta_ll_bound(size_t n);                              /* n(2^(1/n) - 1) */
uint32_t rta_response_time(const rt_task_t *tasks, size_t i); /* 마감을 넘으면 UINT32_MAX */
void     rta_sort_rm(rt_task_t *tasks, size_t n);             /* 주기가 짧은 순(비율 단조) */

#endif
