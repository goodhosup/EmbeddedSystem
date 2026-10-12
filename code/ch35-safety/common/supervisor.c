#include "supervisor.h"

void sup_init(supervisor_t *s)
{
    s->n = 0;
    s->failed = -1;
}

int sup_register(supervisor_t *s, const char *name, uint32_t deadline_ms, uint32_t now_ms)
{
    if (s->n >= SUP_MAX_TASKS || deadline_ms == 0) {
        return -1;
    }
    sup_entry_t *e = &s->e[s->n];
    e->name = name;
    e->deadline_ms = deadline_ms;
    e->last_ms = now_ms;
    return s->n++;
}

void sup_checkin(supervisor_t *s, int id, uint32_t now_ms)
{
    if (id >= 0 && id < s->n) {
        s->e[id].last_ms = now_ms;
    }
}

bool sup_check(supervisor_t *s, uint32_t now_ms)
{
    if (s->failed >= 0) {
        return false;                       /* 이미 실패: 회복을 시도하지 않고 리셋에 맡긴다 */
    }
    for (int i = 0; i < s->n; i++) {
        if (now_ms - s->e[i].last_ms > s->e[i].deadline_ms) {   /* 넘침에 안전한 뺄셈 (24장) */
            s->failed = i;
            return false;
        }
    }
    return true;
}
