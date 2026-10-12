/* 32장: PC 테스트용 NOR Flash 흉내 (STM32F411 섹터 배치, 전원 차단 주입) */
#ifndef FAKE_FLASH_H
#define FAKE_FLASH_H

#include <stdint.h>

void     fake_flash_reset(void);              /* 전체를 0xFF로 */
void     fake_flash_fail_after(int words);    /* words개를 더 쓴 뒤 "전원 차단": 이후 쓰기/지우기 실패 */
void     fake_flash_power_on(void);           /* 전원 복구 */
uint32_t fake_flash_erase_count(uint32_t sector_addr);
uint8_t *fake_flash_mem(uint32_t addr);

#endif
