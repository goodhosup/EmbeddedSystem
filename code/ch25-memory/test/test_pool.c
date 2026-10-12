/* 25장: 메모리 풀과 구조체 배치를 PC에서 시험한다 */
#include <stddef.h>
#include <stdio.h>

#include "pool.h"
#include "layout.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

typedef struct {
    uint8_t  type;
    uint16_t len;
    uint8_t  data[20];
} msg_t;                                   /* 24바이트 → 블록 24바이트 */

#define N_MSG 4
POOL_STORAGE(msg_mem, sizeof(msg_t), N_MSG);

static void test_pool(void)
{
    pool_t p;
    pool_init(&p, msg_mem, sizeof(msg_t), N_MSG);
    CHECK(p.block_size == 24);

    void *b[N_MSG];
    for (int i = 0; i < N_MSG; i++) {
        b[i] = pool_alloc(&p);
        CHECK(b[i] != NULL);
        CHECK(((uintptr_t)b[i] % POOL_ALIGN) == 0);
    }
    CHECK(b[0] == (void *)msg_mem);                       /* 첫 할당은 맨 앞 블록 */
    CHECK((uint8_t *)b[1] - (uint8_t *)b[0] == 24);
    CHECK(pool_alloc(&p) == NULL && p.failures == 1);     /* 다 썼으면 NULL */

    CHECK(pool_free(&p, b[2]));
    CHECK(pool_alloc(&p) == b[2]);                        /* 방금 돌려준 블록을 다시 받는다 */

    CHECK(!pool_free(&p, (uint8_t *)b[1] + 4));            /* 블록 중간 */
    int x;
    CHECK(!pool_free(&p, &x));                            /* 풀 밖 */

    for (int i = 0; i < N_MSG; i++) {
        CHECK(pool_free(&p, b[i]));
    }
    CHECK(p.used == 0 && p.max_used == N_MSG);

    pool_t q;                                             /* 아주 작은 블록도 포인터 크기 이상 */
    static uint64_t tiny_mem[4];
    pool_init(&q, tiny_mem, 1, 2);
    CHECK(q.block_size == POOL_BLOCK_SIZE(1) && q.block_size >= sizeof(void *));
}

static void test_layout(void)
{
    printf("sensor_bad    size %2zu  (id@%zu timestamp@%zu flags@%zu value@%zu)\n",
           sizeof(struct sensor_bad), offsetof(struct sensor_bad, id), offsetof(struct sensor_bad, timestamp),
           offsetof(struct sensor_bad, flags), offsetof(struct sensor_bad, value));
    printf("sensor_good   size %2zu\n", sizeof(struct sensor_good));
    printf("sensor_packed size %2zu\n", sizeof(struct sensor_packed));
    CHECK(sizeof(struct sensor_bad) == 12);
    CHECK(sizeof(struct sensor_good) == 8);
    CHECK(sizeof(struct sensor_packed) == 8);
    CHECK(offsetof(struct sensor_packed, timestamp) == 1);
}

int main(void)
{
    test_pool();
    test_layout();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
