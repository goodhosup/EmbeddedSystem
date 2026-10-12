/* 27장: 수신 ISR이 FreeRTOS 큐에 넣는 USART2 드라이버 */
#ifndef UART_RTOS_H
#define UART_RTOS_H

#include "FreeRTOS.h"
#include "queue.h"

void uart_rtos_init(uint32_t baud, uint32_t pclk1_hz);
void uart_putc(char c);
QueueHandle_t uart_rx_queue(void);     /* 받은 글자(char)가 들어오는 큐 */

#endif
