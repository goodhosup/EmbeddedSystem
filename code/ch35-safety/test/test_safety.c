/* 35장: 감시자, RAM 시험(고장 주입), 방어적 산술의 PC 테스트 */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "supervisor.h"
#include "march.h"
#include "checked.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_supervisor(void)
{
    supervisor_t s;
    sup_init(&s);
    int a = sup_register(&s, "control", 20, 0);
    int b = sup_register(&s, "comm", 600, 0);
    for (uint32_t t = 0; t <= 1000; t += 10) {
        sup_checkin(&s, a, t);                       /* control은 10 ms마다 */
        if (t % 500 == 0) sup_checkin(&s, b, t);      /* comm은 500 ms마다 */
        CHECK(sup_check(&s, t));
    }
    /* control이 멈춤 */
    CHECK(sup_check(&s, 1015));
    CHECK(!sup_check(&s, 1021) && s.failed == a);
    sup_checkin(&s, a, 1022);
    CHECK(!sup_check(&s, 1023));                     /* 한 번 실패하면 회복하지 않는다 */

    /* ms 카운터 넘침 */
    sup_init(&s);
    a = sup_register(&s, "x", 50, 0xFFFFFFF0u);
    CHECK(sup_check(&s, 0x00000010u));              /* 32 ms 지남 */
    CHECK(!sup_check(&s, 0x00000030u));             /* 64 ms 지남 */
}

/* ---- 고장 난 메모리 흉내 ---- */
typedef struct {
    uint32_t mem[64];
    int      stuck_word;        /* 이 워드의 한 비트가 고착 */
    uint32_t stuck_mask, stuck_val;
    int      aggressor, victim; /* aggressor에 쓰면 victim의 비트 0이 뒤집힘 */
} fake_ram_t;

static uint32_t f_rd(void *ctx, size_t i)
{
    fake_ram_t *r = ctx;
    uint32_t v = r->mem[i];
    if ((int)i == r->stuck_word) v = (v & ~r->stuck_mask) | r->stuck_val;
    return v;
}

static void f_wr(void *ctx, size_t i, uint32_t v)
{
    fake_ram_t *r = ctx;
    r->mem[i] = v;
    if ((int)i == r->aggressor) r->mem[r->victim] ^= 1u;
}

static void test_march(void)
{
    fake_ram_t r;
    mem_if_t m = { f_rd, f_wr, &r };

    memset(&r, 0, sizeof r);
    r.stuck_word = r.aggressor = r.victim = -1;
    CHECK(march_c_minus(&m, 64) == -1);              /* 정상 */

    r.stuck_word = 5; r.stuck_mask = 1u << 7; r.stuck_val = 0;           /* 비트 7이 0에 고착 */
    CHECK(march_c_minus(&m, 64) == 5);
    r.stuck_word = 40; r.stuck_mask = 1u << 31; r.stuck_val = 1u << 31;  /* 비트 31이 1에 고착 */
    CHECK(march_c_minus(&m, 64) == 40);

    r.stuck_word = -1; r.aggressor = 3; r.victim = 7;                     /* 결합 고장 */
    long f = march_c_minus(&m, 64);
    CHECK(f == 7);
    r.aggressor = 50; r.victim = 10;                                      /* 뒤쪽에서 앞쪽으로 */
    f = march_c_minus(&m, 64);
    CHECK(f == 10);
}

static void test_checked(void)
{
    int32_t r;
    CHECK(add_i32(1000, 2000, &r) && r == 3000);
    CHECK(!add_i32(INT32_MAX, 1, &r));                 /* 정의되지 않은 동작 없이 넘침을 안다 */
    CHECK(!mul_i32(70000, 70000, &r));
    CHECK(mul_i32(-46340, 46340, &r));
    CHECK(!clamp_i32(1500, 0, 1000, &r) && r == 1000);
    CHECK(clamp_i32(500, 0, 1000, &r) && r == 500);
}

int main(void)
{
    test_supervisor();
    test_march();
    test_checked();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
