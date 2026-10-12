/*
 * 40장: STM32 센서 노드 ↔ ESP32 Wi-Fi 연결부 사이의 UART 프로토콜
 *
 * 프레임  [type][seq][data ...][crc16 lo][crc16 hi]  →  COBS로 0x00을 없애고 끝에 0x00 하나
 *   - 0x00이 프레임 경계이므로, 바이트를 잃거나 깨져도 다음 0x00에서 다시 맞춘다 (14장의 줄 단위와 같은 생각)
 *   - CRC-16/CCITT(13장)로 깨진 프레임을 버린다
 * 신뢰 전송  명령과 OTA 조각은 ACK를 받을 때까지 다시 보낸다 (정지-대기 ARQ)
 *            받는 쪽은 seq로 중복을 걸러 낸다 (ACK를 잃어 다시 온 프레임)
 *            측정값 보고(REPORT)는 잃어도 되므로 ACK 없이 보낸다
 * 이 모듈은 하드웨어와 무관해서 STM32와 ESP32가 같은 파일을 쓰고, PC에서 시험한다.
 */
#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    MSG_ACK = 1,
    MSG_NACK,
    MSG_PING,
    MSG_REPORT,         /* 노드 → 연결부: 측정값 (ACK 없음) */
    MSG_STATUS,         /* 노드 → 연결부: 버전, 슬롯, 리셋 원인 */
    MSG_OTA_BEGIN,      /* 연결부 → 노드: 이미지 크기, 버전 */
    MSG_OTA_CHUNK,      /* 연결부 → 노드: 오프셋 4 B + 데이터 */
    MSG_OTA_END,        /* 연결부 → 노드: 검증하고 시험 부팅 요청 */
};

#define LINK_MAX_DATA   160u
#define LINK_RAW_MAX    (2u + LINK_MAX_DATA + 2u)
#define LINK_FRAME_MAX  (LINK_RAW_MAX + LINK_RAW_MAX / 254u + 2u)   /* COBS 오버헤드 + 구분자 */

typedef struct {
    uint8_t  type;
    uint8_t  seq;
    uint16_t len;
    uint8_t  data[LINK_MAX_DATA];
} link_msg_t;

uint16_t crc16_ccitt(const uint8_t *p, size_t n);

/* 메시지 → 보낼 바이트 (끝의 0x00 포함). 길이를 돌려주고, 공간이 모자라면 0 */
size_t link_encode(const link_msg_t *m, uint8_t *out, size_t cap);

typedef struct {
    uint8_t  buf[LINK_FRAME_MAX];
    size_t   n;
    uint32_t frames, crc_errors, format_errors;
} link_rx_t;

void link_rx_init(link_rx_t *rx);
/* 받은 바이트 하나를 넣는다. 올바른 프레임이 끝나면 out을 채우고 true */
bool link_rx_byte(link_rx_t *rx, uint8_t b, link_msg_t *out);

/* ---- 정지-대기 ARQ (보내는 쪽) ---- */
typedef struct {
    bool     busy;
    uint8_t  seq;           /* 지금 보내는 프레임의 번호 */
    uint8_t  tries;
    uint32_t sent_ms;
    uint32_t timeout_ms;
    uint8_t  max_tries;
    uint32_t retransmits;
} arq_t;

typedef enum { ARQ_IDLE, ARQ_WAIT, ARQ_RESEND, ARQ_FAILED } arq_action_t;

void         arq_init(arq_t *a, uint32_t timeout_ms, uint8_t max_tries);
uint8_t      arq_start(arq_t *a, uint32_t now_ms);           /* 새 프레임: 쓸 seq를 돌려준다 */
bool         arq_on_ack(arq_t *a, uint8_t seq);              /* 기다리던 ACK면 true */
arq_action_t arq_poll(arq_t *a, uint32_t now_ms);            /* 주기적으로: 다시 보낼지 판단 */

/* ---- 받는 쪽 중복 거르기 ---- */
typedef struct { bool have; uint8_t last; } dedup_t;
/* 새 seq면 true(처리), 방금 처리한 seq가 다시 왔으면 false(ACK만 다시) */
bool dedup_accept(dedup_t *d, uint8_t seq);

#endif
