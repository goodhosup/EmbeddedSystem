/*
 * 24장: 세탁기 상태 머신
 *
 *   IDLE --START--> FILLING --FULL--> WASHING --TIMEOUT--> DRAINING
 *        --EMPTY--> SPINNING --TIMEOUT--> DONE --DOOR_OPEN--> IDLE
 *   동작 중 START → PAUSED, PAUSED에서 START → 멈췄던 단계를 처음부터 다시
 *
 * 각 상태에 들어갈 때(entry) 출력을 켜고, 나갈 때(exit) 끈다.
 * 전이 규칙은 washer_dispatch()의 switch 한 곳에만 있다.
 */
#include "washer.h"

static const char *const state_names[WS_COUNT] = {
    "IDLE", "FILLING", "WASHING", "DRAINING", "SPINNING", "DONE", "PAUSED"
};

static const char *const event_names[EV_COUNT] = {
    "NONE", "START", "DOOR_OPEN", "FULL", "EMPTY", "TIMEOUT"
};

const char *washer_state_name(washer_state_t s)
{
    return (s < WS_COUNT) ? state_names[s] : "?";
}

const char *washer_event_name(event_t ev)
{
    return (ev < EV_COUNT) ? event_names[ev] : "?";
}

static bool is_running(washer_state_t s)
{
    return s == WS_FILLING || s == WS_WASHING || s == WS_DRAINING || s == WS_SPINNING;
}

static void on_entry(washer_t *w, washer_state_t s)
{
    const washer_io_t *io = w->io;
    switch (s) {
    case WS_FILLING:
        io->door_lock(true);
        io->valve(true);
        break;
    case WS_WASHING:
        io->motor(MOTOR_WASH);
        io->timer_start(WASH_TIME_MS);
        break;
    case WS_DRAINING:
        io->pump(true);
        break;
    case WS_SPINNING:
        io->pump(true);
        io->motor(MOTOR_SPIN);
        io->timer_start(SPIN_TIME_MS);
        break;
    case WS_DONE:
        io->door_lock(false);
        break;
    default:
        break;
    }
}

static void on_exit(washer_t *w, washer_state_t s)
{
    const washer_io_t *io = w->io;
    switch (s) {
    case WS_FILLING:
        io->valve(false);
        break;
    case WS_WASHING:
        io->motor(MOTOR_STOP);
        io->timer_stop();
        break;
    case WS_DRAINING:
        io->pump(false);
        break;
    case WS_SPINNING:
        io->motor(MOTOR_STOP);
        io->pump(false);
        io->timer_stop();
        break;
    default:
        break;
    }
}

void washer_init(washer_t *w, const washer_io_t *io)
{
    w->io     = io;
    w->state  = WS_IDLE;
    w->resume = WS_IDLE;
    io->valve(false);
    io->pump(false);
    io->motor(MOTOR_STOP);
    io->door_lock(false);
}

void washer_dispatch(washer_t *w, event_t ev)
{
    washer_state_t next = w->state;

    /* 동작 중 어느 단계에서든 START는 일시정지 */
    if (is_running(w->state) && ev == EV_START) {
        w->resume = w->state;
        next = WS_PAUSED;
    } else {
        switch (w->state) {
        case WS_IDLE:
            if (ev == EV_START) next = WS_FILLING;
            break;
        case WS_FILLING:
            if (ev == EV_FULL) next = WS_WASHING;
            break;
        case WS_WASHING:
            if (ev == EV_TIMEOUT) next = WS_DRAINING;
            break;
        case WS_DRAINING:
            if (ev == EV_EMPTY) next = WS_SPINNING;
            break;
        case WS_SPINNING:
            if (ev == EV_TIMEOUT) next = WS_DONE;
            break;
        case WS_DONE:
            if (ev == EV_DOOR_OPEN) next = WS_IDLE;
            break;
        case WS_PAUSED:
            if (ev == EV_START) next = w->resume;
            break;
        default:
            break;
        }
    }

    if (next != w->state) {        /* 전이가 일어날 때만 exit → entry */
        on_exit(w, w->state);
        w->state = next;
        on_entry(w, next);
    }
    /* 그 밖의 이벤트(예: 동작 중 DOOR_OPEN)는 이 상태에서 무시한다 */
}
