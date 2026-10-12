/*
 * 24장: 세탁기 상태 머신 (애플리케이션 계층)
 *
 * 이 모듈은 하드웨어를 직접 모른다. 밸브, 펌프, 모터, 타이머는
 * washer_io_t의 함수 포인터로만 다룬다(의존성 역전, 24.6절).
 * 그래서 같은 코드를 STM32와 PC 테스트(test/)에서 그대로 쓴다.
 */
#ifndef WASHER_H
#define WASHER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WS_IDLE,
    WS_FILLING,
    WS_WASHING,
    WS_DRAINING,
    WS_SPINNING,
    WS_DONE,
    WS_PAUSED,
    WS_COUNT
} washer_state_t;

typedef enum {
    EV_NONE,
    EV_START,          /* 시작/일시정지 버튼 */
    EV_DOOR_OPEN,
    EV_FULL,           /* 수위 센서: 가득 참 */
    EV_EMPTY,          /* 수위 센서: 비었음 */
    EV_TIMEOUT,        /* washer가 요청한 타이머가 끝남 */
    EV_COUNT
} event_t;

enum { MOTOR_STOP = 0, MOTOR_WASH = 1, MOTOR_SPIN = 2 };

typedef struct {
    void (*valve)(bool on);
    void (*pump)(bool on);
    void (*motor)(uint8_t mode);
    void (*door_lock)(bool on);
    void (*timer_start)(uint32_t ms);
    void (*timer_stop)(void);
} washer_io_t;

typedef struct {
    washer_state_t     state;
    washer_state_t     resume;        /* 일시정지에서 돌아갈 상태 */
    const washer_io_t *io;
} washer_t;

#define WASH_TIME_MS  10000u
#define SPIN_TIME_MS   5000u

void        washer_init(washer_t *w, const washer_io_t *io);
void        washer_dispatch(washer_t *w, event_t ev);
const char *washer_state_name(washer_state_t s);
const char *washer_event_name(event_t ev);

#endif
