/*
 * 24장: 협력형 스케줄러와 소프트웨어 타이머 (서비스 계층)
 *
 * 각 태스크는 주기마다 한 번 불려 짧게 일하고 돌아와야 한다.
 * 태스크끼리 서로 끼어들지 않으므로(협력형) 공유 변수 보호가 단순하다.
 */
#ifndef SCHED_H
#define SCHED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *name;
    void      (*fn)(void);
    uint32_t    period_ms;
    uint32_t    next_ms;        /* 다음 실행 시각 */
    uint32_t    max_us;         /* 가장 오래 걸린 실행 시간 (측정용) */
} task_t;

void sched_run_due(task_t *tasks, size_t n, uint32_t now_ms);

/* 시각 비교: 32비트 ms 카운터가 넘쳐 0으로 돌아가도(약 49.7일) 맞게 동작한다 */
static inline bool time_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

typedef struct {
    uint32_t deadline;
    bool     active;
} swtimer_t;

static inline void swtimer_start(swtimer_t *t, uint32_t now, uint32_t ms)
{
    t->deadline = now + ms;
    t->active = true;
}

static inline void swtimer_stop(swtimer_t *t)
{
    t->active = false;
}

/* 만료되었으면 한 번만 true를 돌려주고 멈춘다 */
static inline bool swtimer_expired(swtimer_t *t, uint32_t now)
{
    if (t->active && time_reached(now, t->deadline)) {
        t->active = false;
        return true;
    }
    return false;
}

#endif
