/*
 * 28장: 태스크 집합의 스케줄 가능성을 PC에서 계산한다.
 *   ./rta            예제 태스크 집합 분석
 *   ./rta --test     계산 결과를 알려진 값과 비교
 */
#include <stdio.h>
#include <string.h>

#include "rta.h"

static void report(rt_task_t *ts, size_t n)
{
    double u = rta_utilization(ts, n);
    printf("utilization U = %.3f, Liu-Layland bound for n=%zu: %.3f -> %s\n", u, n, rta_ll_bound(n),
           u <= rta_ll_bound(n) ? "schedulable (sufficient test)" : "bound exceeded, run RTA");
    printf("%-8s %6s %6s %6s %6s %8s\n", "task", "C", "T", "D", "B", "R");
    for (size_t i = 0; i < n; i++) {
        uint32_t r = rta_response_time(ts, i);
        if (r == UINT32_MAX) {
            printf("%-8s %6u %6u %6u %6u %8s\n", ts[i].name, (unsigned)ts[i].C, (unsigned)ts[i].T,
                   (unsigned)ts[i].D, (unsigned)ts[i].B, "MISS");
        } else {
            printf("%-8s %6u %6u %6u %6u %8u\n", ts[i].name, (unsigned)ts[i].C, (unsigned)ts[i].T,
                   (unsigned)ts[i].D, (unsigned)ts[i].B, (unsigned)r);
        }
    }
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int self_test(void)
{
    /* STM32 예제와 같은 태스크 집합 (ms): 응답 시간 1, 5, 18 */
    rt_task_t a[] = { { "log", 10, 50, 50, 0 }, { "ctrl", 1, 5, 5, 0 }, { "comm", 4, 20, 20, 0 } };
    rta_sort_rm(a, 3);
    CHECK(strcmp(a[0].name, "ctrl") == 0 && strcmp(a[2].name, "log") == 0);
    CHECK(rta_response_time(a, 0) == 1);
    CHECK(rta_response_time(a, 1) == 5);
    CHECK(rta_response_time(a, 2) == 18);

    /* 이용률이 상한(n=2: 0.828)을 넘어도 RTA로는 스케줄 가능한 예: U = 0.4 + 0.5 = 0.9 */
    rt_task_t b[] = { { "t1", 2, 5, 5, 0 }, { "t2", 5, 10, 10, 0 } };
    CHECK(rta_utilization(b, 2) > rta_ll_bound(2));
    CHECK(rta_response_time(b, 1) == 9);              /* 5 + 2·ceil(9/5) = 9 */

    /* 스케줄 불가능한 예 */
    rt_task_t c[] = { { "t1", 3, 5, 5, 0 }, { "t2", 3, 7, 7, 0 } };
    CHECK(rta_response_time(c, 1) == UINT32_MAX);

    /* 막힘 시간 B가 더해지는 경우 */
    rt_task_t d[] = { { "t1", 1, 5, 5, 2 }, { "t2", 2, 10, 10, 0 } };
    CHECK(rta_response_time(d, 0) == 3);

    CHECK(rta_ll_bound(1) > 0.999 && rta_ll_bound(1) < 1.001);
    CHECK(rta_ll_bound(3) > 0.779 && rta_ll_bound(3) < 0.780);
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        return self_test();
    }
    rt_task_t ts[] = {
        { "log",  10, 50, 50, 0 },
        { "ctrl",  1,  5,  5, 0 },
        { "comm",  4, 20, 20, 0 },
    };
    size_t n = sizeof ts / sizeof ts[0];
    rta_sort_rm(ts, n);
    report(ts, n);
    return 0;
}
