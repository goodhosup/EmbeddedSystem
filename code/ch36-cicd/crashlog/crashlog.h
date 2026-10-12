/*
 * 36장: 현장 크래시 기록
 *
 * 리셋이나 폴트의 정보(23장의 PC·LR·CFSR, 35장의 리셋 원인)를 Flash의 전용 섹터에 32 B씩 덧붙인다.
 * 다음에 장치에 접속했을 때(UART, 무선) 기록을 꺼내 서버로 보내면, 현장에서 무슨 일이 있었는지 알 수 있다.
 * 섹터가 차면 지우고 처음부터 쓴다: 가장 오래된 기록부터 잃는다.
 * 32장의 flash_if.h(보드: flash_f4.c, PC: fake_flash.c)를 쓴다.
 */
#ifndef CRASHLOG_H
#define CRASHLOG_H

#include <stdint.h>

#define CRASHLOG_ADDR   0x08060000u     /* 섹터 7 (32장 배치에서 비어 있던 곳) */
#define CRASHLOG_SIZE   (128u * 1024u)

enum {                                  /* reason */
    CRASH_WATCHDOG = 1,
    CRASH_HARDFAULT = 2,
    CRASH_ASSERT = 3,
    CRASH_STACK_OVERFLOW = 4,
    CRASH_BROWNOUT = 5,
};

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint16_t reason;
    uint16_t fw_version;    /* (major << 8) | minor */
    uint32_t pc, lr, cfsr;  /* 폴트일 때 (23장) */
    uint32_t uptime_s;
    uint32_t crc;           /* 앞 28 B의 CRC-32 */
} crash_rec_t;              /* 32 B */

int  crashlog_add(uint16_t reason, uint16_t fw_version, uint32_t pc, uint32_t lr, uint32_t cfsr, uint32_t uptime_s);
/* 오래된 것부터 차례로 fn을 부른다. 올바른 기록의 수를 돌려준다 */
int  crashlog_foreach(void (*fn)(const crash_rec_t *r, void *ctx), void *ctx);
int  crashlog_clear(void);

#endif
