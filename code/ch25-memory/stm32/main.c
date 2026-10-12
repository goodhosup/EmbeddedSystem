/*
 * 25장 실습: 스택 측정, MPU 스택 가드, 힙 단편화, 메모리 풀 (NUCLEO-F411RE)
 *
 * 메모리 배치 (이 예제만의 약속)
 *   0x2002 0000  _estack: 스택 꼭대기
 *   0x2001 E000  스택 바닥 (STACK_SIZE = 8 KB)
 *   0x2001 DF00  가드 영역 256 B: MPU로 접근 금지 → 넘치면 MemManage 폴트
 *   그 아래      힙 (_sbrk가 가드 앞에서 멈춘다), .bss, .data
 *
 * 시리얼 터미널(115200)에서 한 글자 명령:
 *   w  스택 최고 수위(high-water mark)와 힙 사용량
 *   d  재귀 20단계 호출 뒤 다시 w
 *   o  끝없는 재귀 → 가드에 닿아 MemManage 폴트
 *   f  힙 단편화 실험 (malloc/free)
 *   p  메모리 풀 실험
 *   a  구조체 크기와 패딩
 *
 * MPU 레지스터: PM0214 4.2절. 영역 크기는 2의 거듭제곱(32 B 이상), 시작 주소는 크기의 배수
 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "pool.h"
#include "layout.h"

extern uint32_t _estack, end;

#define STACK_SIZE   (8u * 1024u)
#define GUARD_SIZE   256u
#define STACK_TOP    ((uintptr_t)&_estack)
#define STACK_BOTTOM (STACK_TOP - STACK_SIZE)
#define GUARD_BASE   (STACK_BOTTOM - GUARD_SIZE)
#define PAINT        0xA5A5A5A5u

/* ---- 힙: 가드 영역 앞에서 멈추는 _sbrk (syscalls.c의 weak 함수를 대신한다) ---- */
static uint8_t *heap_end;

void *_sbrk(int incr)
{
    if (heap_end == NULL) {
        heap_end = (uint8_t *)&end;
    }
    uint8_t *prev = heap_end;
    if ((uintptr_t)(heap_end + incr) > GUARD_BASE) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return prev;
}

/* ---- 스택 칠하기: 아직 쓰지 않은 스택을 알려진 값으로 채워 두고, 나중에 얼마나 지워졌는지 본다 ---- */
static void stack_paint(void)
{
    uint32_t *p = (uint32_t *)STACK_BOTTOM;
    uint32_t *sp;
    __asm volatile ("mov %0, sp" : "=r" (sp));
    while (p < sp - 16) {            /* 지금 쓰고 있는 부분(과 여유 64 B)은 건드리지 않는다 */
        *p++ = PAINT;
    }
}

static uint32_t stack_high_water(void)
{
    const uint32_t *p = (const uint32_t *)STACK_BOTTOM;
    while (p < (const uint32_t *)STACK_TOP && *p == PAINT) {
        p++;
    }
    return (uint32_t)(STACK_TOP - (uintptr_t)p);    /* 가장 깊이 썼던 바이트 수 */
}

/* ---- MPU: 스택 바로 아래 256 B를 접근 금지로 ---- */
static void mpu_stack_guard(void)
{
    MPU_CTRL = 0;                                    /* 설정하는 동안 끈다 */
    MPU_RNR  = 0;                                    /* 영역 0 */
    MPU_RBAR = GUARD_BASE;                           /* 256의 배수여야 한다 */
    MPU_RASR = MPU_RASR_XN | (0u << MPU_RASR_AP_Pos) /* AP = 000: 누구도 접근 불가 */
             | MPU_RASR_S | MPU_RASR_C
             | (7u << MPU_RASR_SIZE_Pos)             /* 2^(7+1) = 256 B */
             | MPU_RASR_ENABLE;
    MPU_CTRL = MPU_CTRL_PRIVDEFENA | MPU_CTRL_ENABLE; /* 나머지 영역은 기본 메모리 맵 */
    __asm volatile ("dsb\n isb" ::: "memory");
    SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA;              /* HardFault 대신 MemManage로 */
}

/* 스택이 가드에 닿으면 SP가 이미 가드 안에 있으므로, 출력하기 전에 SP를 스택 꼭대기로 옮긴다 */
__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile (
        "mrs   r0, msp         \n"     /* 폴트 순간의 SP (기록용) */
        "ldr   r1, =_estack    \n"
        "msr   msp, r1         \n"     /* 새 스택에서 출력 함수를 실행 */
        "b     memfault_report \n"
    );
}

__attribute__((used, noreturn)) void memfault_report(uint32_t old_sp)
{
    uint32_t cfsr  = SCB_CFSR;
    uint32_t mmfar = SCB_MMFAR;
    printf("\n*** MemManage fault: stack overflow? ***\n");
    printf("CFSR  = 0x%08lX%s%s%s\n", (unsigned long)cfsr,
           (cfsr & (1u << 1)) ? " DACCVIOL" : "",
           (cfsr & (1u << 4)) ? " MSTKERR" : "",
           (cfsr & (1u << 7)) ? " MMARVALID" : "");
    if (cfsr & (1u << 7)) {
        printf("MMFAR = 0x%08lX (guard 0x%08lX ~ 0x%08lX)\n",
               (unsigned long)mmfar, (unsigned long)GUARD_BASE, (unsigned long)(STACK_BOTTOM - 1u));
    }
    printf("SP    = 0x%08lX\n", (unsigned long)old_sp);
    __asm volatile ("bkpt #0");
    for (;;) {
    }
}

/* ---- 명령 ---- */
static uint32_t recurse(uint32_t n)
{
    volatile uint8_t pad[64];
    pad[0] = (uint8_t)n;
    return n == 0 ? pad[0] : recurse(n - 1) + pad[0];
}

static void cmd_watermark(void)
{
    printf("stack: %lu / %lu bytes used (high-water)\n",
           (unsigned long)stack_high_water(), (unsigned long)STACK_SIZE);
    printf("heap : 0x%08lX ~ 0x%08lX (%lu bytes from sbrk), limit 0x%08lX\n",
           (unsigned long)(uintptr_t)&end, (unsigned long)(uintptr_t)heap_end,
           (unsigned long)(heap_end ? (uintptr_t)heap_end - (uintptr_t)&end : 0u),
           (unsigned long)GUARD_BASE);
}

#define FRAG_N 160     /* 힙(약 120 KB)을 1 KB 블록으로 다 채울 만큼 */
static void cmd_fragment(void)
{
    static void *blk[FRAG_N];
    unsigned got = 0;
    /* 1 KB 블록을 힙이 찰 때까지 받는다 */
    for (unsigned i = 0; i < FRAG_N; i++) {
        blk[i] = malloc(1024);
        if (blk[i] != NULL) {
            got++;
        }
    }
    void *extra = malloc(2048);
    printf("allocated %u x 1 KB, then 2 KB: %s\n", got, extra ? "ok" : "FAILED");
    free(extra);
    /* 하나 건너 하나씩 돌려준다: 빈 공간의 합은 크지만 1 KB 조각들로 흩어진다.
       맨 위 블록(got - 1)은 남겨 두어 힙 끝과 붙은 빈 공간이 생기지 않게 한다 */
    unsigned freed = 0;
    for (unsigned i = 0; i + 1u < got; i += 2) {
        if (blk[i] != NULL) {
            free(blk[i]);
            blk[i] = NULL;
            freed++;
        }
    }
    void *big = malloc(2048);
    printf("freed %u KB in 1 KB pieces, malloc(2 KB): %s\n", freed, big ? "ok" : "FAILED (fragmented)");
    free(big);
    for (unsigned i = 0; i < FRAG_N; i++) {
        free(blk[i]);
        blk[i] = NULL;
    }
    big = malloc(2048);
    printf("after freeing all, malloc(2 KB): %s\n", big ? "ok" : "FAILED");
    free(big);
}

typedef struct {
    uint8_t  type;
    uint16_t len;
    uint8_t  data[20];
} msg_t;

#define N_MSG 8
POOL_STORAGE(msg_mem, sizeof(msg_t), N_MSG);
static pool_t msg_pool;

static void cmd_pool(void)
{
    msg_t *m[N_MSG + 1];
    for (unsigned i = 0; i <= N_MSG; i++) {
        m[i] = pool_alloc(&msg_pool);
    }
    printf("pool: block %u B x %u, 9th alloc %s\n", (unsigned)msg_pool.block_size, N_MSG,
           m[N_MSG] ? "ok" : "NULL (pool empty)");
    for (unsigned i = 0; i < N_MSG; i++) {
        pool_free(&msg_pool, m[i]);
    }
    printf("pool: used %u, max used %u, failures %u\n", (unsigned)msg_pool.used,
           (unsigned)msg_pool.max_used, (unsigned)msg_pool.failures);
}

static void cmd_layout(void)
{
    printf("sensor_bad    %u B (timestamp@%u, value@%u)\n", (unsigned)sizeof(struct sensor_bad),
           (unsigned)offsetof(struct sensor_bad, timestamp), (unsigned)offsetof(struct sensor_bad, value));
    printf("sensor_good   %u B\n", (unsigned)sizeof(struct sensor_good));
    printf("sensor_packed %u B (timestamp@%u)\n", (unsigned)sizeof(struct sensor_packed),
           (unsigned)offsetof(struct sensor_packed, timestamp));
}

int main(void)
{
    stack_paint();
    mpu_stack_guard();
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    pool_init(&msg_pool, msg_mem, sizeof(msg_t), N_MSG);

    printf("\nch25 memory: w=watermark d=deep o=overflow f=fragment p=pool a=layout\n");
    for (;;) {
        int ch = uart2_getc();
        switch (ch) {
        case 'w': cmd_watermark(); break;
        case 'd': printf("recurse(20) = %lu\n", (unsigned long)recurse(20)); cmd_watermark(); break;
        case 'o': printf("recurse forever...\n"); printf("%lu\n", (unsigned long)recurse(100000)); break;
        case 'f': cmd_fragment(); break;
        case 'p': cmd_pool(); break;
        case 'a': cmd_layout(); break;
        default: break;
        }
    }
}
