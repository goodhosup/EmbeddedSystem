#include <string.h>

#include "dsp.h"

#if defined(__ARM_FEATURE_DSP) && __ARM_FEATURE_DSP
#include <arm_acle.h>
#define HAVE_SMLALD 1
#endif

void fir_f32(const float *x, const float *h, float *y, size_t n, size_t taps)
{
    for (size_t i = 0; i < n; i++) {
        float acc = 0.0f;
        const float *xp = &x[i + taps - 1];
        for (size_t k = 0; k < taps; k++) {
            acc += h[k] * xp[-(ptrdiff_t)k];          /* FPU의 곱셈-덧셈 (VFMA로 묶일 수 있다) */
        }
        y[i] = acc;
    }
}

void fir_q15_ref(const q15_t *x, const q15_t *h, q15_t *y, size_t n, size_t taps)
{
    for (size_t i = 0; i < n; i++) {
        int64_t acc = 0;                              /* Q30을 많이 더해도 넘치지 않게 64비트 */
        const q15_t *xp = &x[i + taps - 1];
        for (size_t k = 0; k < taps; k++) {
            acc += (int32_t)h[k] * xp[-(ptrdiff_t)k];
        }
        y[i] = q15_sat((int32_t)((acc + (1 << 14)) >> 15));
    }
}

/*
 * 두 탭씩: 32비트 워드 하나에 Q15 두 개를 담아 한 명령으로 곱해 더한다.
 *   SMLALD acc, a, b : acc += a.lo × b.lo + a.hi × b.hi  (64비트 누산)
 * x는 거꾸로 읽어야 하므로 계수 쪽을 거꾸로 저장해 두는 대신,
 * x[j], x[j+1]과 h[k+1], h[k]를 짝지어 순서를 맞춘다.
 */
static inline uint32_t load2(const q15_t *p)
{
    uint32_t w;
    memcpy(&w, p, 4);                                 /* 정렬되지 않았을 수 있다: LDR은 허용(25장) */
    return w;
}

static inline uint32_t pack(q15_t lo, q15_t hi)
{
    return (uint16_t)lo | ((uint32_t)(uint16_t)hi << 16);
}

void fir_q15_fast(const q15_t *x, const q15_t *h, q15_t *y, size_t n, size_t taps)
{
    for (size_t i = 0; i < n; i++) {
        int64_t acc = 0;
        const q15_t *xp = &x[i];                      /* 가장 오래된 표본부터 */
        for (size_t k = 0; k < taps; k += 2) {
            /* y[i] = Σ h[k] · x[i + taps - 1 - k]: 뒤에서부터 두 개씩 */
            size_t j = taps - 2 - k;                  /* x[i+j], x[i+j+1] ↔ h[k+1], h[k] */
            uint32_t xx = load2(&xp[j]);
            uint32_t hh = pack(h[k + 1], h[k]);
#ifdef HAVE_SMLALD
            acc = __smlald(xx, hh, acc);
#else
            acc += (int32_t)(int16_t)(xx & 0xFFFFu) * (int16_t)(hh & 0xFFFFu)
                 + (int32_t)(int16_t)(xx >> 16) * (int16_t)(hh >> 16);
#endif
        }
        y[i] = q15_sat((int32_t)((acc + (1 << 14)) >> 15));
    }
}
