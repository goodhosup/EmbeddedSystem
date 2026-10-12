/* USART2 (NUCLEO 가상 COM 포트) 공통 모듈: 14장에서 만든 것을 정리했다 */
#ifndef UART2_H
#define UART2_H

#include <stdint.h>

void uart2_init(uint32_t baud, uint32_t pclk1_hz);
void uart2_putc(char c);
int  uart2_getc(void);          /* 받은 문자, 없으면 -1 */

#endif
