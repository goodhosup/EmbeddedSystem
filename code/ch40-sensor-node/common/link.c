#include <string.h>

#include "link.h"

uint16_t crc16_ccitt(const uint8_t *p, size_t n)      /* 다항식 0x1021, 초기값 0xFFFF (13장) */
{
    uint16_t crc = 0xFFFFu;
    while (n--) {
        crc ^= (uint16_t)(*p++ << 8);
        for (int i = 0; i < 8; i++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* COBS: 0x00을 없앤다. 각 블록 앞의 코드 바이트가 "다음 0x00까지의 거리"를 알려 준다 */
static size_t cobs_encode(const uint8_t *in, size_t n, uint8_t *out)
{
    size_t code_pos = 0, o = 1;
    uint8_t code = 1;
    for (size_t i = 0; i < n; i++) {
        if (in[i] == 0) {
            out[code_pos] = code;
            code_pos = o++;
            code = 1;
        } else {
            out[o++] = in[i];
            if (++code == 0xFF) {
                out[code_pos] = code;
                code_pos = o++;
                code = 1;
            }
        }
    }
    out[code_pos] = code;
    return o;
}

static size_t cobs_decode(const uint8_t *in, size_t n, uint8_t *out, size_t cap)
{
    size_t i = 0, o = 0;
    while (i < n) {
        uint8_t code = in[i++];
        if (code == 0) return 0;
        for (uint8_t k = 1; k < code; k++) {
            if (i >= n || o >= cap) return 0;
            out[o++] = in[i++];
        }
        if (code != 0xFF && i < n) {
            if (o >= cap) return 0;
            out[o++] = 0;
        }
    }
    return o;
}

size_t link_encode(const link_msg_t *m, uint8_t *out, size_t cap)
{
    uint8_t raw[LINK_RAW_MAX];
    if (m->len > LINK_MAX_DATA || cap < LINK_FRAME_MAX) return 0;
    raw[0] = m->type;
    raw[1] = m->seq;
    memcpy(&raw[2], m->data, m->len);
    uint16_t crc = crc16_ccitt(raw, 2u + m->len);
    raw[2 + m->len] = (uint8_t)crc;
    raw[3 + m->len] = (uint8_t)(crc >> 8);
    size_t n = cobs_encode(raw, 4u + m->len, out);
    out[n++] = 0x00;                                  /* 프레임 끝 */
    return n;
}

void link_rx_init(link_rx_t *rx)
{
    memset(rx, 0, sizeof *rx);
}

bool link_rx_byte(link_rx_t *rx, uint8_t b, link_msg_t *out)
{
    if (b != 0x00) {
        if (rx->n < sizeof rx->buf) {
            rx->buf[rx->n++] = b;
        } else {
            rx->n = sizeof rx->buf + 1;               /* 넘침 표시: 이 프레임은 버린다 */
        }
        return false;
    }
    size_t n = rx->n;
    rx->n = 0;
    if (n == 0) return false;                         /* 빈 프레임 (연속된 0x00) */
    if (n > sizeof rx->buf) { rx->format_errors++; return false; }

    uint8_t raw[LINK_RAW_MAX + 1];
    size_t m = cobs_decode(rx->buf, n, raw, sizeof raw);
    if (m < 4 || m - 4 > LINK_MAX_DATA) { rx->format_errors++; return false; }
    uint16_t crc = (uint16_t)(raw[m - 2] | (raw[m - 1] << 8));
    if (crc16_ccitt(raw, m - 2) != crc) { rx->crc_errors++; return false; }

    out->type = raw[0];
    out->seq = raw[1];
    out->len = (uint16_t)(m - 4);
    memcpy(out->data, &raw[2], out->len);
    rx->frames++;
    return true;
}

void arq_init(arq_t *a, uint32_t timeout_ms, uint8_t max_tries)
{
    memset(a, 0, sizeof *a);
    a->timeout_ms = timeout_ms;
    a->max_tries = max_tries;
}

uint8_t arq_start(arq_t *a, uint32_t now_ms)
{
    a->seq++;
    a->busy = true;
    a->tries = 1;
    a->sent_ms = now_ms;
    return a->seq;
}

bool arq_on_ack(arq_t *a, uint8_t seq)
{
    if (a->busy && seq == a->seq) {
        a->busy = false;
        return true;
    }
    return false;                                     /* 늦게 온 옛 ACK는 무시 */
}

arq_action_t arq_poll(arq_t *a, uint32_t now_ms)
{
    if (!a->busy) return ARQ_IDLE;
    if (now_ms - a->sent_ms < a->timeout_ms) return ARQ_WAIT;
    if (a->tries >= a->max_tries) {
        a->busy = false;
        return ARQ_FAILED;
    }
    a->tries++;
    a->retransmits++;
    a->sent_ms = now_ms;
    return ARQ_RESEND;
}

bool dedup_accept(dedup_t *d, uint8_t seq)
{
    if (d->have && d->last == seq) return false;
    d->have = true;
    d->last = seq;
    return true;
}
