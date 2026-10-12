#include "pool.h"

void pool_init(pool_t *p, void *mem, size_t block_size, size_t count)
{
    p->base       = (uint8_t *)mem;
    p->block_size = POOL_BLOCK_SIZE(block_size);
    p->count      = count;
    p->used       = 0;
    p->max_used   = 0;
    p->failures   = 0;
    p->free_list  = NULL;

    /* 뒤에서부터 연결해 두면 첫 할당이 맨 앞 블록을 준다 */
    for (size_t i = count; i-- > 0; ) {
        void **blk = (void **)(p->base + i * p->block_size);
        *blk = p->free_list;
        p->free_list = blk;
    }
}

void *pool_alloc(pool_t *p)
{
    void **blk = (void **)p->free_list;
    if (blk == NULL) {
        p->failures++;
        return NULL;
    }
    p->free_list = *blk;             /* 리스트 머리를 다음 빈 블록으로 */
    p->used++;
    if (p->used > p->max_used) {
        p->max_used = p->used;
    }
    return blk;
}

bool pool_free(pool_t *p, void *blk)
{
    uint8_t *b = (uint8_t *)blk;
    size_t span = p->block_size * p->count;
    /* 다른 곳의 포인터나 블록 중간을 가리키는 포인터를 거른다 */
    if (b < p->base || b >= p->base + span || (size_t)(b - p->base) % p->block_size != 0) {
        return false;
    }
    *(void **)blk = p->free_list;
    p->free_list = blk;
    p->used--;
    return true;
}
