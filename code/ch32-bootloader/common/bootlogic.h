/*
 * 32장: 어느 슬롯으로 부팅할지 정하는 규칙 (하드웨어와 무관, PC에서 시험)
 *
 *   새 이미지를 받으면       pending = 새 슬롯, tries = 0        (boot_request)
 *   부트로더                 pending이 있으면 tries를 올리고 pending으로 시험 부팅
 *                            tries가 BOOT_MAX_TRIES에 이르렀는데도 확정되지 않았으면 롤백
 *   새 앱이 스스로 확인 후   active = pending, pending 없음     (boot_confirm)
 * MCUboot의 test/confirm 방식과 같은 생각이다.
 */
#ifndef BOOTLOGIC_H
#define BOOTLOGIC_H

#include <stdbool.h>
#include "bootmeta.h"

#define BOOT_MAX_TRIES 3u

typedef enum {
    BOOT_NORMAL,            /* 확정된 슬롯으로 */
    BOOT_TRIAL,             /* 새 이미지를 시험 */
    BOOT_ROLLBACK,          /* 새 이미지가 확정되지 못함 → 이전 슬롯으로 */
    BOOT_PENDING_INVALID,   /* 새 이미지가 깨짐 → 이전 슬롯으로 */
    BOOT_RECOVERY,          /* 확정 슬롯이 깨짐 → 다른 올바른 슬롯으로 */
    BOOT_NO_IMAGE,          /* 실행할 이미지가 없음 */
} boot_event_t;

typedef struct {
    int          slot;      /* 부팅할 슬롯, 없으면 -1 */
    boot_event_t event;
    bool         save;      /* 상태가 바뀌었으니 점프 전에 meta_save 해야 함 */
} boot_decision_t;

boot_decision_t boot_decide(boot_state_t *st, const bool valid[2]);
bool            boot_confirm(boot_state_t *st, int running_slot);   /* 바뀌었으면 true */
void            boot_request(boot_state_t *st, int new_slot);
const char     *boot_event_str(boot_event_t e);

#endif
