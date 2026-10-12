#ifndef LINK_UART_H
#define LINK_UART_H
#include <stddef.h>
#include <stdint.h>
void link_uart_init(uint32_t baud, uint32_t pclk2_hz);
void link_uart_write(const uint8_t *p, size_t n);
int  link_uart_getc(void);
#endif
