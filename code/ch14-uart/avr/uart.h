/* 인터럽트 방식 UART (수신·송신 링 버퍼) */
#ifndef UART_H
#define UART_H

#include <stdint.h>

void    uart_init(uint32_t baud);
void    uart_putc(char c);               /* 송신 버퍼가 꽉 차면 빌 때까지 기다린다 */
int     uart_getc(void);                 /* 받은 문자, 없으면 -1 */
uint8_t uart_rx_overflow(void);          /* 수신 버퍼가 넘친 적이 있으면 1 (읽으면 지워짐) */
void    uart_stdio_init(void);           /* printf를 UART로 */

#endif
