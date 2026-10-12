/* 30장: USART2 송신은 DMA, 수신은 인터럽트 → 큐 (FreeRTOS) */
#ifndef UART_DMA_H
#define UART_DMA_H

#include <stddef.h>
#include "FreeRTOS.h"
#include "queue.h"

void          uart_dma_init(uint32_t baud, uint32_t pclk1_hz);   /* 스케줄러 시작 전에 */
void          uart_dma_write(const char *s, size_t n);          /* 태스크에서만. 보내기가 끝날 때까지 블록 */
void          log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
QueueHandle_t uart_rx_queue(void);
void          uart_panic_puts(const char *s);                    /* 폴링 출력: 폴트, 훅용 */

#endif
