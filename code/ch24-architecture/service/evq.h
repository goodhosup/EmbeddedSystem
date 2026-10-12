/*
 * 24장: 이벤트 큐 (서비스 계층)
 *
 * ISR과 메인 루프 양쪽에서 evq_put()을 부를 수 있다.
 * 한 바이트짜리 이벤트를 2의 거듭제곱 크기 링 버퍼에 넣는다(14장).
 */
#ifndef EVQ_H
#define EVQ_H

#include <stdbool.h>
#include <stdint.h>

#define EVQ_SIZE 16u                /* 2의 거듭제곱 */

typedef struct {
    volatile uint8_t  buf[EVQ_SIZE];
    volatile uint32_t head, tail;
    volatile uint32_t dropped;      /* 가득 차서 버린 이벤트 수 */
} evq_t;

void evq_init(evq_t *q);
bool evq_put(evq_t *q, uint8_t ev);
int  evq_get(evq_t *q);             /* 꺼낸 이벤트, 비었으면 -1 */

#endif
