/*
 * 32장: Flash 접근 인터페이스
 *   보드: stm32/flash_f4.c (FLASH 레지스터)   PC 테스트: test/fake_flash.c (NOR Flash 흉내)
 * NOR Flash의 규칙: 지우면 모든 비트가 1(0xFF), 쓰기는 1을 0으로만 바꿀 수 있다. 다시 1로 만들려면 섹터째 지운다.
 */
#ifndef FLASH_IF_H
#define FLASH_IF_H

#include <stdint.h>

int         flash_erase(uint32_t sector_addr);                         /* 0이면 성공 */
int         flash_write(uint32_t addr, const void *data, uint32_t len); /* 4바이트 정렬, 4의 배수 */
const void *flash_map(uint32_t addr);                                  /* 읽기: 주소 → 포인터 */

#endif
