/*
 * 34장: 고정소수점(Q15)과 FIR 필터 세 가지 구현
 *
 *   Q15: int16_t 하나로 -1.0 ~ +0.999969를 나타낸다. 값 = 정수 / 32768
 *   fir_f32        float (Cortex-M4F의 FPU)
 *   fir_q15_ref    Q15, 이해하기 쉬운 기준 구현
 *   fir_q15_fast   Q15, 두 탭씩 한 번에 (Cortex-M4의 SIMD 명령 SMLALD), 결과는 ref와 비트 단위로 같다
 *
 * 입력 x는 앞쪽에 taps-1개의 과거 표본을 포함한다: y[i] = Σ h[k] · x[i + taps - 1 - k]
 */
#ifndef DSP_H
#define DSP_H

#include <stddef.h>
#include <stdint.h>

typedef int16_t q15_t;

#define Q15_ONE_MINUS 0x7FFF
#define Q15_MINUS_ONE (-32768)

static inline q15_t q15_sat(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (q15_t)v;
}

static inline q15_t q15_from_float(float f)
{
    float s = f * 32768.0f;
    return q15_sat((int32_t)(s >= 0 ? s + 0.5f : s - 0.5f));    /* 반올림 */
}

static inline float q15_to_float(q15_t q)
{
    return (float)q / 32768.0f;
}

/* Q15 × Q15 = Q30 → 반올림해 Q15로. -1 × -1 = +1은 표현할 수 없으므로 포화 */
static inline q15_t q15_mul(q15_t a, q15_t b)
{
    int32_t p = (int32_t)a * b;           /* Q30 */
    return q15_sat((p + (1 << 14)) >> 15);
}

void fir_f32(const float *x, const float *h, float *y, size_t n, size_t taps);
void fir_q15_ref(const q15_t *x, const q15_t *h, q15_t *y, size_t n, size_t taps);
void fir_q15_fast(const q15_t *x, const q15_t *h, q15_t *y, size_t n, size_t taps);   /* taps는 짝수 */

#endif
