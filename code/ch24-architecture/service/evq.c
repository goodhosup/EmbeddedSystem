#include "evq.h"
#include "port.h"

void evq_init(evq_t *q)
{
    q->head = q->tail = 0;
    q->dropped = 0;
}

bool evq_put(evq_t *q, uint8_t ev)
{
    /* 여러 ISR과 메인이 함께 넣을 수 있으므로 head 갱신을 보호한다 (7장) */
    uint32_t key = port_crit_enter();
    uint32_t next = (q->head + 1u) & (EVQ_SIZE - 1u);
    bool ok = (next != q->tail);
    if (ok) {
        q->buf[q->head] = ev;
        q->head = next;
    } else {
        q->dropped++;
    }
    port_crit_exit(key);
    return ok;
}

int evq_get(evq_t *q)
{
    /* 꺼내는 쪽은 메인 루프 하나뿐이므로 tail은 보호하지 않아도 된다 */
    if (q->tail == q->head) {
        return -1;
    }
    uint8_t ev = q->buf[q->tail];
    q->tail = (q->tail + 1u) & (EVQ_SIZE - 1u);
    return ev;
}
