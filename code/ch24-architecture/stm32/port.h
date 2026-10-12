/* 24장: 서비스 계층이 쓰는 플랫폼 함수 (STM32F411) */
#ifndef PORT_H
#define PORT_H

#include "stm32f411_regs.h"

static inline uint32_t port_crit_enter(void) { return irq_save(); }
static inline void     port_crit_exit(uint32_t key) { irq_restore(key); }

/* DWT 사이클 카운터(34장)로 µs 단위 시각. 16 MHz HSI 기준 */
static inline uint32_t port_us(void) { return DWT_CYCCNT / 16u; }

#endif
