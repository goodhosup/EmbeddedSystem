/*
 * 40장: 링크 프로토콜 PC 테스트
 *   1) 인코딩/디코딩: 0x00이 섞인 데이터, 최대 길이, 깨진 프레임
 *   2) 잡음 채널 시뮬레이션: 바이트의 0.1%를 잃고 0.1%를 깨뜨리는 UART 위에서 OTA 조각 300개를
 *      ARQ로 보내 정확히 한 번씩, 순서대로 도착하는지
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_codec(void)
{
    link_msg_t m = { MSG_OTA_CHUNK, 7, 0, {0} }, r;
    uint8_t f[LINK_FRAME_MAX];
    link_rx_t rx;

    CHECK(crc16_ccitt((const uint8_t *)"123456789", 9) == 0x29B1);     /* CRC-16/CCITT-FALSE 확인 값 */

    for (unsigned len = 0; len <= LINK_MAX_DATA; len += 40) {
        m.len = (uint16_t)len;
        for (unsigned i = 0; i < len; i++) m.data[i] = (uint8_t)(i % 3 == 0 ? 0 : i);   /* 0x00이 많이 */
        size_t n = link_encode(&m, f, sizeof f);
        CHECK(n > 0 && f[n - 1] == 0);
        for (size_t i = 0; i + 1 < n; i++) CHECK(f[i] != 0);                         /* 중간에 0x00 없음 */
        link_rx_init(&rx);
        bool got = false;
        for (size_t i = 0; i < n; i++) got |= link_rx_byte(&rx, f[i], &r);
        CHECK(got && r.type == m.type && r.seq == 7 && r.len == len && memcmp(r.data, m.data, len) == 0);
    }

    /* 비트 하나가 깨지면 버리고, 다음 프레임은 정상으로 받는다 */
    m.len = 10;
    size_t n = link_encode(&m, f, sizeof f);
    link_rx_init(&rx);
    f[3] ^= 0x10;
    if (f[3] == 0) f[3] = 0x10;                   /* 이 시험에서는 0x00으로 바뀌지 않게 */
    bool got = false;
    for (size_t i = 0; i < n; i++) got |= link_rx_byte(&rx, f[i], &r);
    CHECK(!got && rx.crc_errors + rx.format_errors == 1);
    f[3] ^= 0x10;
    n = link_encode(&m, f, sizeof f);
    for (size_t i = 0; i < n; i++) got |= link_rx_byte(&rx, f[i], &r);
    CHECK(got);
}

/* ---- 잡음 채널 ---- */
#define CH_CAP 4096
typedef struct { uint8_t q[CH_CAP]; size_t head, tail; } chan_t;
static unsigned noise_seed = 12345;
static int rnd(int n) { noise_seed = noise_seed * 1103515245u + 12345u; return (int)((noise_seed >> 16) % (unsigned)n); }

static void ch_put(chan_t *c, const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int r = rnd(1000);
        if (r < 1) continue;                        /* 0.1% 바이트 잃음 */
        uint8_t b = p[i];
        if (r < 2) b ^= (uint8_t)(1u << rnd(8));    /* 0.1% 비트 깨짐 (실제 UART보다 훨씬 나쁜 조건) */
        c->q[c->head++ % CH_CAP] = b;
    }
}
static int ch_get(chan_t *c) { return c->tail == c->head ? -1 : c->q[c->tail++ % CH_CAP]; }

static void test_noisy_ota(void)
{
    chan_t to_node = {0}, to_bridge = {0};
    link_rx_t node_rx, bridge_rx;
    link_rx_init(&node_rx);
    link_rx_init(&bridge_rx);
    arq_t arq;
    arq_init(&arq, 50, 10);
    dedup_t dd = {0};

    enum { CHUNKS = 300 };
    static uint8_t image[CHUNKS * 128], written[CHUNKS * 128];
    for (size_t i = 0; i < sizeof image; i++) image[i] = (uint8_t)rnd(256);
    unsigned applied = 0, duplicates = 0, next = 0;
    uint8_t f[LINK_FRAME_MAX];
    link_msg_t cur, r;

    for (uint32_t now = 0; now < 600000 && next < CHUNKS; now++) {   /* 1 ms 단위로 흉내 */
        if (!arq.busy) {                                   /* 연결부: 다음 조각 */
            cur.type = MSG_OTA_CHUNK;
            cur.len = 4 + 128;
            uint32_t off = next * 128u;
            memcpy(cur.data, &off, 4);
            memcpy(&cur.data[4], &image[off], 128);
            cur.seq = arq_start(&arq, now);
            ch_put(&to_node, f, link_encode(&cur, f, sizeof f));
        } else {
            arq_action_t a = arq_poll(&arq, now);
            if (a == ARQ_RESEND) ch_put(&to_node, f, link_encode(&cur, f, sizeof f));
            CHECK(a != ARQ_FAILED);
        }
        for (int i = 0; i < 20; i++) {                     /* 노드: 받고, 쓰고, ACK */
            int b = ch_get(&to_node);
            if (b < 0) break;
            if (link_rx_byte(&node_rx, (uint8_t)b, &r) && r.type == MSG_OTA_CHUNK) {
                if (dedup_accept(&dd, r.seq)) {
                    uint32_t off;
                    memcpy(&off, r.data, 4);
                    CHECK(off == applied * 128u);          /* 순서대로, 빠짐없이 */
                    memcpy(&written[off], &r.data[4], 128);
                    applied++;
                } else {
                    duplicates++;                          /* ACK를 잃어 다시 온 것: 쓰지 않는다 */
                }
                link_msg_t ack = { MSG_ACK, r.seq, 0, {0} };
                ch_put(&to_bridge, f, link_encode(&ack, f, sizeof f));
            }
        }
        for (int i = 0; i < 20; i++) {                     /* 연결부: ACK 받기 */
            int b = ch_get(&to_bridge);
            if (b < 0) break;
            if (link_rx_byte(&bridge_rx, (uint8_t)b, &r) && r.type == MSG_ACK && arq_on_ack(&arq, r.seq)) {
                next++;
            }
        }
    }
    CHECK(applied == CHUNKS);
    CHECK(memcmp(image, written, sizeof image) == 0);
    printf("noisy link: %u chunks, %lu retransmits, %u duplicates dropped, rx crc errors %lu, format errors %lu\n",
           applied, (unsigned long)arq.retransmits, duplicates,
           (unsigned long)node_rx.crc_errors, (unsigned long)node_rx.format_errors);
}

int main(void)
{
    test_codec();
    test_noisy_ota();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
