/*
 * 32장: 부트 상태 (어느 슬롯이 확정되었고, 어느 슬롯을 시험 중인가)
 *
 * Flash에 덮어쓸 수 없으므로 16 B 기록을 뒤에 계속 덧붙이고, 일련번호가 가장 큰 올바른 기록을 쓴다.
 * 섹터가 차면 다른 섹터를 지우고 거기에 이어 쓴다(섹터 두 개를 번갈아). 그래서 지우는 도중
 * 전원이 끊겨도 다른 섹터에 이전 기록이 남아 있다.
 */
#ifndef BOOTMETA_H
#define BOOTMETA_H

#include <stdint.h>

#define SLOT_NONE 0xFFu

typedef struct {
    uint8_t active;     /* 확정된(잘 동작한다고 확인된) 슬롯: 0 = A, 1 = B */
    uint8_t pending;    /* 시험 중인 새 슬롯, 없으면 SLOT_NONE */
    uint8_t tries;      /* pending으로 부팅한 횟수 */
    uint8_t reserved;
} boot_state_t;

/* 가장 최근 기록을 읽는다. 기록이 하나도 없으면 기본값(A 확정)을 채우고 -1 */
int meta_load(boot_state_t *st);
/* 새 기록을 덧붙인다. 0이면 성공 */
int meta_save(const boot_state_t *st);
/* 두 섹터를 모두 지운다 (공장 초기화, 테스트용) */
int meta_erase_all(void);

#endif
