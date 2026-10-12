/*
 * 24장: 세탁기 상태 머신과 스케줄러를 PC에서 시험한다.
 * 보드 대신 가짜 washer_io_t가 출력 상태를 기록한다.
 */
#include <stdio.h>
#include <string.h>

#include "washer.h"
#include "evq.h"
#include "sched.h"

static bool valve, pump, locked, timer_on;
static uint8_t motor;
static uint32_t timer_ms;

static void f_valve(bool on)      { valve = on; }
static void f_pump(bool on)       { pump = on; }
static void f_motor(uint8_t m)    { motor = m; }
static void f_lock(bool on)       { locked = on; }
static void f_tstart(uint32_t ms) { timer_on = true; timer_ms = ms; }
static void f_tstop(void)         { timer_on = false; }

static const washer_io_t fake = { f_valve, f_pump, f_motor, f_lock, f_tstart, f_tstop };

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_full_cycle(void)
{
    washer_t w;
    washer_init(&w, &fake);
    CHECK(w.state == WS_IDLE && !locked);

    washer_dispatch(&w, EV_TIMEOUT);              /* IDLE에서는 무시 */
    CHECK(w.state == WS_IDLE);

    washer_dispatch(&w, EV_START);
    CHECK(w.state == WS_FILLING && valve && locked);

    washer_dispatch(&w, EV_FULL);
    CHECK(w.state == WS_WASHING && !valve && motor == MOTOR_WASH && timer_on && timer_ms == WASH_TIME_MS);

    washer_dispatch(&w, EV_DOOR_OPEN);            /* 잠겨 있으므로 무시 */
    CHECK(w.state == WS_WASHING);

    washer_dispatch(&w, EV_TIMEOUT);
    CHECK(w.state == WS_DRAINING && motor == MOTOR_STOP && pump && !timer_on);

    washer_dispatch(&w, EV_EMPTY);
    CHECK(w.state == WS_SPINNING && pump && motor == MOTOR_SPIN && timer_ms == SPIN_TIME_MS);

    washer_dispatch(&w, EV_TIMEOUT);
    CHECK(w.state == WS_DONE && !pump && motor == MOTOR_STOP && !locked);

    washer_dispatch(&w, EV_DOOR_OPEN);
    CHECK(w.state == WS_IDLE);
}

static void test_pause_resume(void)
{
    washer_t w;
    washer_init(&w, &fake);
    washer_dispatch(&w, EV_START);
    washer_dispatch(&w, EV_FULL);                 /* WASHING */
    washer_dispatch(&w, EV_START);                /* 일시정지 */
    CHECK(w.state == WS_PAUSED && motor == MOTOR_STOP && !timer_on && locked);
    washer_dispatch(&w, EV_TIMEOUT);              /* 멈춘 동안의 이벤트는 무시 */
    CHECK(w.state == WS_PAUSED);
    washer_dispatch(&w, EV_START);                /* 다시 시작 */
    CHECK(w.state == WS_WASHING && motor == MOTOR_WASH && timer_on);
}

static void test_evq(void)
{
    evq_t q;
    evq_init(&q);
    CHECK(evq_get(&q) == -1);
    for (unsigned i = 0; i < EVQ_SIZE - 1u; i++) {
        CHECK(evq_put(&q, (uint8_t)(i + 1u)));
    }
    CHECK(!evq_put(&q, 99) && q.dropped == 1);    /* 한 칸은 비워 두므로 15개가 최대 */
    for (unsigned i = 0; i < EVQ_SIZE - 1u; i++) {
        CHECK(evq_get(&q) == (int)(i + 1u));
    }
    CHECK(evq_get(&q) == -1);
}

static int runs_a, runs_b;
static void task_a(void) { runs_a++; }
static void task_b(void) { runs_b++; }

static void test_sched(void)
{
    task_t t[] = { { "a", task_a, 10, 0, 0 }, { "b", task_b, 100, 0, 0 } };
    for (uint32_t now = 0; now < 1000; now++) {
        sched_run_due(t, 2, now);
    }
    CHECK(runs_a == 100 && runs_b == 10);

    /* 카운터가 넘치는 순간에도 시각 비교가 맞는가 */
    CHECK(time_reached(5u, 0xFFFFFFF0u));
    CHECK(!time_reached(0xFFFFFFF0u, 5u));
    swtimer_t st;
    swtimer_start(&st, 0xFFFFFFF0u, 32);
    CHECK(!swtimer_expired(&st, 0xFFFFFFFFu));
    CHECK(swtimer_expired(&st, 16u));
    CHECK(!swtimer_expired(&st, 17u));            /* 한 번만 */
}

int main(void)
{
    test_full_cycle();
    test_pause_resume();
    test_evq();
    test_sched();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
