/* UART 송신 (14장에서 자세히 다룬다) */
#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_put_fixed1(int16_t value10);   /* 234 → "23.4", -52 → "-5.2" */

#endif
