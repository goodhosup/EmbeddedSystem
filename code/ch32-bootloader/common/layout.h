/*
 * 32장: STM32F411RE(512 KB) Flash 배치
 *
 *   섹터 0~1  0x0800 0000  32 KB   부트로더
 *   섹터 2    0x0800 8000  16 KB   부트 상태 기록 0 ┐ 번갈아 쓴다 (bootmeta.c)
 *   섹터 3    0x0800 C000  16 KB   부트 상태 기록 1 ┘
 *   섹터 4    0x0801 0000  64 KB   (비어 있음: 앱 데이터용)
 *   섹터 5    0x0802 0000 128 KB   슬롯 A  ┐ 머리말 512 B + 앱 (벡터 테이블은 +0x200)
 *   섹터 6    0x0804 0000 128 KB   슬롯 B  ┘
 *   섹터 7    0x0806 0000 128 KB   (비어 있음)
 * 섹터 크기가 고르지 않은 것은 RM0383 3.3절 표 "Flash module organization" 그대로다.
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>

#define FLASH_BASE_ADDR   0x08000000u
#define FLASH_TOTAL       (512u * 1024u)

#define BOOT_ADDR         0x08000000u
#define META0_ADDR        0x08008000u
#define META1_ADDR        0x0800C000u
#define META_SECTOR_SIZE  (16u * 1024u)

#define SLOT_A_ADDR       0x08020000u
#define SLOT_B_ADDR       0x08040000u
#define SLOT_SIZE         (128u * 1024u)
#define SLOT_COUNT        2u

#define RAM_START         0x20000000u
#define RAM_END           0x20020000u

static inline uint32_t slot_addr(int slot)
{
    return slot == 0 ? SLOT_A_ADDR : SLOT_B_ADDR;
}

#endif
