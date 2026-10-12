#include "thermostat.h"

#define VALID_MIN (-4000)
#define VALID_MAX 8500

static void set_heater(thermostat_t *th, bool on)
{
    th->io->heater(th->io->ctx, on);
}

void thermo_init(thermostat_t *th, const thermo_io_t *io, int32_t setpoint_c100, int32_t hyst_c100)
{
    th->io = io;
    th->setpoint_c100 = setpoint_c100;
    th->hyst_c100 = hyst_c100;
    th->state = TH_IDLE;
    th->fails = 0;
    th->goods = 0;
    set_heater(th, false);                         /* 처음에는 반드시 꺼진 상태에서 시작 */
}

static bool read_valid(thermostat_t *th, int32_t *t)
{
    if (th->io->read_c100(th->io->ctx, t) != 0) {
        return false;
    }
    return *t >= VALID_MIN && *t <= VALID_MAX;     /* 센서가 내는 이상한 값도 실패로 본다 */
}

void thermo_step(thermostat_t *th)
{
    int32_t t;
    bool ok = read_valid(th, &t);

    if (th->state == TH_FAULT) {
        th->goods = ok ? (uint8_t)(th->goods + 1u) : 0u;
        if (th->goods >= THERMO_RECOVER_COUNT) {
            th->state = TH_IDLE;
            th->goods = 0;
            th->fails = 0;
        }
        return;                                    /* 회복한 이번 주기에는 아직 히터를 켜지 않는다 */
    }

    if (!ok) {
        if (++th->fails >= THERMO_FAIL_LIMIT) {
            th->state = TH_FAULT;
            th->goods = 0;
            set_heater(th, false);
        }
        return;                                    /* 실패 횟수가 적으면 지금 상태를 유지 */
    }
    th->fails = 0;

    if (th->state == TH_IDLE && t < th->setpoint_c100 - th->hyst_c100) {
        th->state = TH_HEATING;
        set_heater(th, true);
    } else if (th->state == TH_HEATING && t > th->setpoint_c100 + th->hyst_c100) {
        th->state = TH_IDLE;
        set_heater(th, false);
    }
}

bool thermo_set_setpoint(thermostat_t *th, int32_t c100)
{
    if (c100 < THERMO_SETPOINT_MIN || c100 > THERMO_SETPOINT_MAX) {
        return false;
    }
    th->setpoint_c100 = c100;
    return true;
}

th_state_t thermo_state(const thermostat_t *th)
{
    return th->state;
}
