/*
 * 31장: 히터 온도 조절기 (테스트 주도 개발로 만든 모듈)
 *
 *   - 온도가 (설정값 - 히스테리시스)보다 낮으면 히터 켜기, (설정값 + 히스테리시스)보다 높으면 끄기
 *   - 센서 읽기가 3번 연속 실패(또는 -40~85 °C 밖의 값)하면 FAULT: 히터를 끄고 유지
 *   - FAULT에서 정상 값을 3번 연속 받으면 다시 동작
 *
 * 하드웨어는 thermo_io_t의 함수 포인터로만 다룬다(24장). 그래서 PC와 보드에서 같은 시험을 돌린다.
 */
#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int  (*read_c100)(void *ctx, int32_t *t_c100);   /* 0이면 성공 */
    void (*heater)(void *ctx, bool on);
    void *ctx;
} thermo_io_t;

typedef enum { TH_IDLE, TH_HEATING, TH_FAULT } th_state_t;

#define THERMO_FAIL_LIMIT     3
#define THERMO_RECOVER_COUNT  3
#define THERMO_SETPOINT_MIN   500       /* 5.00 °C */
#define THERMO_SETPOINT_MAX   3500      /* 35.00 °C */

typedef struct {
    const thermo_io_t *io;
    int32_t    setpoint_c100;
    int32_t    hyst_c100;
    th_state_t state;
    uint8_t    fails;          /* 연속 실패 수 */
    uint8_t    goods;          /* FAULT 중 연속 정상 수 */
} thermostat_t;

void       thermo_init(thermostat_t *th, const thermo_io_t *io, int32_t setpoint_c100, int32_t hyst_c100);
void       thermo_step(thermostat_t *th);                       /* 주기마다 부른다 */
bool       thermo_set_setpoint(thermostat_t *th, int32_t c100);  /* 범위 밖이면 false, 바꾸지 않음 */
th_state_t thermo_state(const thermostat_t *th);

#endif
