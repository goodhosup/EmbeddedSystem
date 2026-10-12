/*
 * SPI 마스터 인터페이스 (16장): 모드 0, MSB 먼저, 8비트
 * 보드마다 spi_avr.c, spi_stm32.c가 같은 이름으로 구현한다.
 */
#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void    spi_init(uint32_t sclk_hz);
uint8_t spi_xfer(uint8_t out);      /* 1바이트 보내면서 1바이트 받기 */
void    spi_cs_low(void);           /* 장치 선택 */
void    spi_cs_high(void);          /* 선택 해제: 명령의 끝 */

#endif
