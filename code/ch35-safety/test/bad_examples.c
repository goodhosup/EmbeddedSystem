/*
 * 35장: 정적 분석기가 무엇을 찾는지 보여 주려고 일부러 버그를 넣은 코드 (제품에 쓰지 말 것)
 *   make analyze 로 GCC -fanalyzer의 경고를 본다.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t len; uint8_t data[16]; } frame_t;

/* 1. 할당 실패를 확인하지 않음 */
frame_t *frame_new(void)
{
    frame_t *f = malloc(sizeof *f);
    f->len = 0;
    return f;
}

/* 2. 해제한 뒤 사용 */
uint8_t frame_first(frame_t *f)
{
    free(f);
    return f->data[0];
}

/* 3. 모든 경로에서 초기화하지 않은 변수 */
int32_t scale(int mode, int32_t x)
{
    int32_t k;
    if (mode == 1) {
        k = 10;
    } else if (mode == 2) {
        k = 100;
    }
    return x * k;
}

/* 4. 해제하지 않고 잃어버림 (누수) */
int count_bytes(const uint8_t *src, size_t n)
{
    uint8_t *copy = malloc(n);
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, src, n);
    return (int)n;
}
