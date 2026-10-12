/*
 * 35장: 방어적 산술 — 넘치면 조용히 틀린 값을 쓰지 않고 실패를 알린다.
 * GCC/Clang의 __builtin_*_overflow는 넘침 여부를 정의된 동작으로 알려 준다.
 */
#ifndef CHECKED_H
#define CHECKED_H

#include <stdbool.h>
#include <stdint.h>

static inline bool add_i32(int32_t a, int32_t b, int32_t *out)
{
    return !__builtin_add_overflow(a, b, out);       /* true = 성공 */
}

static inline bool mul_i32(int32_t a, int32_t b, int32_t *out)
{
    return !__builtin_mul_overflow(a, b, out);
}

/* 범위를 벗어난 값은 경계로 붙잡고, 붙잡았는지 알려 준다 */
static inline bool clamp_i32(int32_t v, int32_t lo, int32_t hi, int32_t *out)
{
    if (v < lo) { *out = lo; return false; }
    if (v > hi) { *out = hi; return false; }
    *out = v;
    return true;
}

#endif
