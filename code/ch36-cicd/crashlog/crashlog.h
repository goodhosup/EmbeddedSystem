/*
 * 36장: 현장 크래시 기록
 *
 * 리셋이나 폴트의 정보(23장의 PC·LR·CFSR, 35장의 리셋 원인)를 Flash의 전용 섹터에 32 B씩 덧붙인다.
 * 다음에 장치에 접속했을 때(UART, 무선) 기록을 꺼내 서버로 보내면, 현장에서 무슨 일이 있었는지 알 수 있다.
 * 섹터가 차면(4096건) 섹터 전체를 지우고 처음부터 쓴다: 그때까지의 기록을 모두 잃는다.
 * 오래된 것만 잃게 하려면 섹터 두 개를 번갈아 쓴다(32장 bootmeta와 같은 방식).
 * 128 KB 섹터 지우기는 1~2초 걸리고 그동안 CPU가 멈춘다: 워치독을 켜기 전(부팅 직후)에 지운다.
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
