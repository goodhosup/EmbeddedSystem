/* 30장: PC 테스트용 가짜 SD 카드 (SPI 모드 프로토콜을 흉내 낸다) */
#ifndef FAKE_SD_H
#define FAKE_SD_H

#include <stdbool.h>
#include <stdint.h>

#define FAKE_SD_SECTORS 16384u          /* 8 MB, SDHC (CSD v2, C_SIZE = 15) */

void     fake_sd_reset(void);           /* 전원을 껐다 켬 (데이터는 남는다) */
void     fake_sd_insert(bool present);  /* false: 카드 뽑힘 */
uint32_t fake_sd_writes(void);          /* CMD24 횟수 */
uint8_t *fake_sd_image(void);           /* 카드 내용 (8 MB) */

#endif
