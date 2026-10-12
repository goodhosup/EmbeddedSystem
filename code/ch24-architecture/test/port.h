/* 24장: PC 테스트용 플랫폼 함수 (인터럽트가 없으므로 비어 있다) */
#ifndef PORT_H
#define PORT_H

#include <stdint.h>

static inline uint32_t port_crit_enter(void) { return 0; }
static inline void     port_crit_exit(uint32_t key) { (void)key; }
static inline uint32_t port_cycles(void) { return 0; }
#define PORT_CYCLES_PER_US 1u

#endif
