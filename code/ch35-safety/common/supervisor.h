/*
 * 35장: 태스크 감시자 (워치독을 "모든 태스크가 살아 있을 때만" 갱신한다)
 *
 * 워치독을 메인 루프 한 곳에서 무조건 갱신하면, 루프는 돌지만 태스크 하나가 멈춘 상황을 잡지 못한다.
 * 각 태스크가 주기적으로 sup_checkin()을 부르고, sup_check()가 모두 기한 안에 왔다고 할 때만 갱신한다.
 */
#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <stdbool.h>
#include <stdint.h>

#define SUP_MAX_TASKS 8

typedef struct {
    const char *name;
    uint32_t    deadline_ms;   /* 이 시간 안에 한 번은 체크인해야 한다 */
    uint32_t    last_ms;
} sup_entry_t;

typedef struct {
    sup_entry_t e[SUP_MAX_TASKS];
    int         n;
    int         failed;        /* 처음 기한을 넘긴 태스크 번호, 없으면 -1 */
} supervisor_t;

void sup_init(supervisor_t *s);
int  sup_register(supervisor_t *s, const char *name, uint32_t deadline_ms, uint32_t now_ms);  /* 번호, 실패 -1 */
void sup_checkin(supervisor_t *s, int id, uint32_t now_ms);
/* 모든 태스크가 기한 안이면 true (→ 워치독 갱신). 한 번 실패하면 계속 false: 리셋을 기다린다 */
bool sup_check(supervisor_t *s, uint32_t now_ms);

#endif
