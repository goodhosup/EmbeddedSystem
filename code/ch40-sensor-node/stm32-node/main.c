/*
 * 40장: 센서 노드 (NUCLEO-F411RE, 32장 부트로더 위의 슬롯 A/B 앱)
 *
 *   - 10초마다 BMP280 온도를 MSG_REPORT로 ESP32에 보낸다 (ACK 없음)
 *   - ESP32가 보내는 OTA_BEGIN / CHUNK / END를 ota_writer로 받아 다른 슬롯에 쓰고,
 *     서명(33장)을 확인한 뒤 시험 부팅을 요청한다
 *   - 시험 부팅 중이면 자가 진단(센서 응답 + ESP32 응답)을 통과했을 때 확정한다 (32장)
 *   - 부트로더가 켠 워치독을 메인 루프에서 갱신한다 (35장)
 * 디버그 출력은 USART2 (ST-LINK 가상 COM).
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "link_uart.h"
#include "link.h"
#include "ota_writer.h"
#include "layout.h"
#include "image.h"
#include "bootmeta.h"
#include "bootlogic.h"
#include "i2c.h"
#include "bmp280.h"
#include "secure_image.h"
#include "pubkey.h"

#ifndef FW_VERSION
#define FW_VERSION 0x010000u
#endif
#define MIN_SEC_COUNTER 1u

static volatile uint32_t ms;
void SysTick_Handler(void) { ms++; }

static link_rx_t rx;
static dedup_t   dedup;
static ota_t     ota;
static uint8_t   tx_seq;

static int running_slot(void) { return SCB_VTOR == SLOT_A_ADDR + IMG_HDR_SIZE ? 0 : 1; }

static void send(uint8_t type, uint8_t seq, const void *data, uint16_t len)
{
    link_msg_t m = { type, seq, len, {0} };
    memcpy(m.data, data, len);
    uint8_t f[LINK_FRAME_MAX];
    link_uart_write(f, link_encode(&m, f, sizeof f));
}

/* 33장의 서명 확인을 ota_writer가 부르는 모양으로 */
static int verify_signed(uint32_t slot_base, uint32_t size)
{
    (void)size;
    sv_result_t r = secure_verify((const uint8_t *)slot_base, SLOT_SIZE, signing_pubkey, MIN_SEC_COUNTER);
    printf("[ota] signature: %s\n", secure_result_str(r));
    return r == SV_OK ? 0 : -1;
}

static void handle(const link_msg_t *m, bool *bridge_alive)
{
    static int8_t last_result;                          /* 다시 온 프레임에는 지난번 결과를 다시 알린다 */
    if (m->type == MSG_ACK || m->type == MSG_REPORT) return;
    *bridge_alive = true;
    if (m->type == MSG_PING) {                          /* 여러 번 받아도 같은 일: 중복 거르기 밖에서 */
        int8_t ok = 0;
        send(MSG_ACK, m->seq, &ok, 1);
        return;
    }
    int8_t result = last_result;
    if (dedup_accept(&dedup, m->seq)) {                 /* OTA 명령: 다시 온 프레임은 처리하지 않고 결과만 다시 */
        uint32_t a = 0, b = 0;
        if (m->len >= 4) memcpy(&a, m->data, 4);
        if (m->len >= 8) memcpy(&b, &m->data[4], 4);
        switch (m->type) {
        case MSG_PING:      break;
        case MSG_OTA_BEGIN: result = (int8_t)ota_begin(&ota, running_slot() ^ 1, a, b);
                            printf("[ota] begin %lu bytes v%06lX: %d\n", (unsigned long)a, (unsigned long)b, result); break;
        case MSG_OTA_CHUNK: result = (int8_t)ota_chunk(&ota, a, &m->data[4], (uint16_t)(m->len - 4)); break;
        case MSG_OTA_END:   result = (int8_t)ota_end(&ota);
                            printf("[ota] end: %d%s\n", result, result == 0 ? " (reset to try the new image)" : ""); break;
        default:            result = -100; break;
        }
        last_result = result;
    }
    send(result == 0 ? MSG_ACK : MSG_NACK, m->seq, &result, 1);
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    link_uart_init(115200, 16000000u);
    i2c_init(100000);
    link_rx_init(&rx);
    ota_init(&ota, verify_signed);
    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    boot_state_t st;
    meta_load(&st);
    bool trial = (st.pending == (uint8_t)running_slot());
    printf("\n[node] v%06lX slot %c%s\n", (unsigned long)FW_VERSION, running_slot() ? 'B' : 'A', trial ? " (TRIAL)" : "");

    bmp280_t bmp;
    bool sensor_ok = bmp280_init(&bmp, BMP280_ADDR_LOW) == 0;
    bool bridge_alive = false;
    uint32_t next_report = 0, seq = 0;

    for (;;) {
        IWDG->KR = IWDG_KEY_RELOAD;                     /* 32장 부트로더가 켠 워치독 */

        int c;
        link_msg_t m;
        while ((c = link_uart_getc()) >= 0) {
            if (link_rx_byte(&rx, (uint8_t)c, &m)) handle(&m, &bridge_alive);
        }

        if ((int32_t)(ms - next_report) >= 0) {
            next_report = ms + 10000u;
            int32_t t = 0;
            sensor_ok = sensor_ok && bmp280_read_temp_c100(&bmp, &t) == 0;
            uint8_t rep[13];
            uint32_t up = ms / 1000u, ver = FW_VERSION;
            memcpy(rep, &seq, 4); memcpy(&rep[4], &up, 4); memcpy(&rep[8], &t, 4);
            rep[12] = sensor_ok ? 0 : 1;
            send(MSG_REPORT, tx_seq++, rep, sizeof rep);
            (void)ver;
            seq++;
        }

        /* 자가 진단: 시험 부팅 중이면 센서와 ESP32가 모두 응답할 때 확정 (32장) */
        if (trial && sensor_ok && bridge_alive) {
            meta_load(&st);
            if (boot_confirm(&st, running_slot()) && meta_save(&st) == 0) {
                printf("[node] self-test passed: image confirmed\n");
            }
            trial = false;
        }
    }
}
