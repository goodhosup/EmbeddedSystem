/* 이동 평균 (9장의 코드를 보드와 상관없는 모듈로 정리). 31장에서 PC로 시험한다 */
#ifndef AVERAGE_H
#define AVERAGE_H

#include <stdint.h>

#define AVG_MAX_N 32u

typedef struct {
    uint16_t buf[AVG_MAX_N];
    uint32_t sum;
    uint8_t  n;          /* 창 크기 (1 ~ AVG_MAX_N) */
    uint8_t  idx;
    uint8_t  count;      /* 지금까지 들어온 개수 (n보다 작을 수 있음) */
} average_t;

void     average_init(average_t *a, uint8_t n);
uint16_t average_add(average_t *a, uint16_t sample);   /* 새 값을 넣고 평균을 돌려준다 */

#endif
