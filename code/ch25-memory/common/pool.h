/*
 * 25장: 고정 크기 메모리 풀
 *
 * 같은 크기의 블록 여러 개를 미리 잡아 두고, 빈 블록을 연결 리스트(free list)로 관리한다.
 * 할당과 해제가 항상 O(1)이고 단편화가 생기지 않는다.
 * 이 모듈은 인터럽트로부터 보호하지 않는다. ISR과 함께 쓰면 호출하는 쪽에서 임계 구역으로 감싼다.
 */
#ifndef POOL_H
#define POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 블록은 8바이트 경계에 맞춘다 (double, uint64_t도 담을 수 있게) */
#define POOL_ALIGN 8u
#define POOL_BLOCK_SIZE(sz) ((((sz) < sizeof(void *) ? sizeof(void *) : (sz)) + POOL_ALIGN - 1u) & ~(size_t)(POOL_ALIGN - 1u))

/* 풀 저장 공간을 정적으로 잡는다. uint64_t 배열이라 8바이트 정렬이 보장된다 */
#define POOL_STORAGE(name, size, count) \
    static uint64_t name[(POOL_BLOCK_SIZE(size) * (count)) / sizeof(uint64_t)]

typedef struct {
    void    *free_list;     /* 빈 블록의 연결 리스트 (빈 블록의 첫 워드에 다음 블록 주소를 둔다) */
    uint8_t *base;
    size_t   block_size;
    size_t   count;
    size_t   used;
    size_t   max_used;      /* 가장 많이 쓴 블록 수: 풀 크기를 정하는 근거 */
    size_t   failures;      /* 비어서 실패한 횟수 */
} pool_t;

void  pool_init(pool_t *p, void *mem, size_t block_size, size_t count);
void *pool_alloc(pool_t *p);                 /* 비었으면 NULL */
bool  pool_free(pool_t *p, void *blk);       /* 이 풀의 블록이 아니면 false */

#endif
