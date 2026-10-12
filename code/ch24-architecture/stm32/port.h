/* 24장: 서비스 계층이 쓰는 플랫폼 함수 (STM32F411) */
#ifndef PORT_H
#define PORT_H

#include "stm32f411_regs.h"

static inline uint32_t port_crit_enter(void) { return irq_save(); }
static inline void     port_crit_exit(uint32_t key) { irq_restore(key); }

/* DWT 사이클 카운터(34장). 실행 시간은 사이클 차이를 먼저 구한 뒤 µs로 나눈다 */
static inline uint32_t port_cycles(void) { return DWT_CYCCNT; }
#define PORT_CYCLES_PER_US 16u      /* 16 MHz HSI */

#endif
