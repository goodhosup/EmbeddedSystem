/* 34장: Q15 연산과 FIR 구현들의 PC 테스트 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dsp.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

#define N    256
#define TAPS 32

static void test_q15_basics(void)
{
    CHECK(q15_from_float(0.5f) == 16384);
    CHECK(q15_from_float(-1.0f) == -32768);
    CHECK(q15_from_float(1.0f) == 32767);                     /* +1은 없다: 포화 */
    CHECK(q15_mul(16384, 16384) == 8192);                     /* 0.5 × 0.5 = 0.25 */
    CHECK(q15_mul(-32768, -32768) == 32767);                  /* -1 × -1 = +1 → 포화 */
    CHECK(q15_mul(-16384, 16384) == -8192);
    CHECK(q15_sat(40000) == 32767 && q15_sat(-40000) == -32768);
}

static void test_fir(void)
{
    static float xf[N + TAPS - 1], hf[TAPS], yf[N];
    static q15_t xq[N + TAPS - 1], hq[TAPS], yr[N], yfast[N];

    /* 저역 통과 필터 비슷한 계수(합 ≈ 0.9)와 사인파 + 잡음 */
    srand(1);
    for (int k = 0; k < TAPS; k++) {
        float w = 0.5f - 0.5f * cosf(2.0f * 3.14159265f * (float)k / (TAPS - 1));
        hf[k] = 0.9f * w / 15.5f;
        hq[k] = q15_from_float(hf[k]);
    }
    for (int i = 0; i < N + TAPS - 1; i++) {
        float v = 0.6f * sinf(2.0f * 3.14159265f * (float)i / 40.0f) + 0.2f * ((float)rand() / RAND_MAX - 0.5f);
        xq[i] = q15_from_float(v);
        xf[i] = q15_to_float(xq[i]);                          /* 같은 입력을 쓰도록 */
    }

    fir_f32(xf, hf, yf, N, TAPS);
    fir_q15_ref(xq, hq, yr, N, TAPS);
    fir_q15_fast(xq, hq, yfast, N, TAPS);

    int same = 1;
    float maxerr = 0;
    for (int i = 0; i < N; i++) {
        if (yr[i] != yfast[i]) same = 0;
        float e = fabsf(q15_to_float(yr[i]) - yf[i]);
        if (e > maxerr) maxerr = e;
    }
    CHECK(same);                                              /* 빠른 구현 = 기준 구현 (비트 단위) */
    printf("max |q15 - float| = %.6f (%.1f LSB)\n", maxerr, maxerr * 32768.0f);
    CHECK(maxerr < 32.0f / 32768.0f);                         /* 계수 양자화 오차 정도 */

    /* 포화: 큰 입력과 합이 1을 넘는 계수 */
    for (int k = 0; k < TAPS; k++) hq[k] = 8192;              /* 0.25 × 32 = 8.0 */
    for (int i = 0; i < N + TAPS - 1; i++) xq[i] = 32767;
    fir_q15_ref(xq, hq, yr, N, TAPS);
    fir_q15_fast(xq, hq, yfast, N, TAPS);
    CHECK(yr[0] == 32767 && yfast[0] == 32767);
}

int main(void)
{
    test_q15_basics();
    test_fir();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
