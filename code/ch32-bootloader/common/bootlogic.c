#include "bootlogic.h"

boot_decision_t boot_decide(boot_state_t *st, const bool valid[2])
{
    boot_decision_t d = { -1, BOOT_NORMAL, false };

    if (st->pending != SLOT_NONE) {
        if (st->pending > 1u || !valid[st->pending]) {
            st->pending = SLOT_NONE;                      /* 받다가 끊긴 이미지 등 */
            st->tries = 0;
            d.event = BOOT_PENDING_INVALID;
            d.save = true;
        } else if (st->tries >= BOOT_MAX_TRIES) {
            st->pending = SLOT_NONE;                      /* 여러 번 기회를 줬는데 확정하지 못함 */
            st->tries = 0;
            d.event = BOOT_ROLLBACK;
            d.save = true;
        } else {
            st->tries++;                                  /* 점프하기 전에 저장해야 한다: 새 앱이 멈춰도 센다 */
            d.slot = st->pending;
            d.event = BOOT_TRIAL;
            d.save = true;
            return d;
        }
    }

    if (st->active <= 1u && valid[st->active]) {
        d.slot = st->active;
        return d;
    }
    for (int s = 0; s < 2; s++) {                         /* 확정 슬롯이 깨졌다면 다른 올바른 슬롯이라도 */
        if (valid[s]) {
            st->active = (uint8_t)s;
            d.slot = s;
            d.event = BOOT_RECOVERY;
            d.save = true;
            return d;
        }
    }
    d.event = BOOT_NO_IMAGE;
    return d;
}

bool boot_confirm(boot_state_t *st, int running_slot)
{
    if (st->pending == (uint8_t)running_slot) {
        st->active = (uint8_t)running_slot;
        st->pending = SLOT_NONE;
        st->tries = 0;
        return true;
    }
    return false;                                         /* 이미 확정된 슬롯에서 실행 중 */
}

void boot_request(boot_state_t *st, int new_slot)
{
    st->pending = (uint8_t)new_slot;
    st->tries = 0;
}

const char *boot_event_str(boot_event_t e)
{
    static const char *const s[] = { "normal", "trial", "rollback", "pending image invalid",
                                     "recovery", "no valid image" };
    return (unsigned)e < sizeof s / sizeof s[0] ? s[e] : "?";
}
