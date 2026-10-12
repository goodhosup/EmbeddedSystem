#include "average.h"

void average_init(average_t *a, uint8_t n)
{
    if (n == 0) {
        n = 1;
    } else if (n > AVG_MAX_N) {
        n = AVG_MAX_N;
    }
    for (uint32_t i = 0; i < AVG_MAX_N; i++) {
        a->buf[i] = 0;
    }
    a->sum = 0;
    a->n = n;
    a->idx = 0;
    a->count = 0;
}

uint16_t average_add(average_t *a, uint16_t sample)
{
    a->sum -= a->buf[a->idx];
    a->buf[a->idx] = sample;
    a->sum += sample;
    a->idx = (uint8_t)((a->idx + 1u) % a->n);
    if (a->count < a->n) {
        a->count++;                       /* 처음 n개까지는 들어온 개수로 나눈다 */
    }
    return (uint16_t)(a->sum / a->count);
}
