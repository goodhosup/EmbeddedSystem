/*
 * 32장: 업데이트를 받을 수 있는 앱 (슬롯 A 또는 B에서 실행)
 *
 * 명령 (115200 bps)
 *   s  상태: 지금 슬롯, 버전, 확정 여부, 두 슬롯의 이미지
 *   c  지금 이미지를 확정 (시험 부팅 중일 때)
 *   u  새 이미지 받기 (tools/send_update.py가 보낸다) → 다른 슬롯에 쓰고 검증 → 시험 요청
 *   x  멈춘 척하기: 워치독이 리셋한다 (확정하지 않은 새 이미지라면 3번 뒤 롤백)
 *   r  리셋
 * 부트로더가 켜 둔 독립 워치독(약 4초)을 메인 루프에서 계속 갱신한다.
 */
#include <stdbool.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "layout.h"
#include "flash_if.h"
#include "image.h"
#include "bootmeta.h"
#include "bootlogic.h"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif

#define CHUNK 256u

static void puts_(const char *s)
{
    while (*s) {
        if (*s == '\n') uart2_putc('\r');
        uart2_putc(*s++);
    }
}

static void put_u32(uint32_t v)
{
    char b[11];
    int i = 10;
    b[i] = '\0';
    do { b[--i] = (char)('0' + v % 10u); v /= 10u; } while (v);
    puts_(&b[i]);
}

static void wdg_kick(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;
}

static int running_slot(void)
{
    return (SCB_VTOR == SLOT_A_ADDR + IMG_HDR_SIZE) ? 0 : 1;
}

static void print_version(uint32_t v)
{
    put_u32(v >> 16); uart2_putc('.'); put_u32((v >> 8) & 0xFFu); uart2_putc('.'); put_u32(v & 0xFFu);
}

static void status(void)
{
    boot_state_t st;
    meta_load(&st);
    int me = running_slot();
    puts_("app " APP_VERSION_STR " running from slot ");
    uart2_putc(me ? 'B' : 'A');
    if (st.pending == (uint8_t)me) {
        puts_(" (TRIAL, boot #"); put_u32(st.tries); puts_(" of 3, press c to confirm)\n");
    } else {
        puts_(" (confirmed)\n");
    }
    for (int s = 0; s < 2; s++) {
        image_hdr_t h;
        img_result_t r = image_check(s, &h);
        puts_(s ? "  slot B: " : "  slot A: ");
        puts_(image_result_str(r));
        if (r == IMG_OK) { puts_(", v"); print_version(h.version); }
        puts_("\n");
    }
}

/* 시간 제한 있는 한 바이트 받기 (DWT 사이클 카운터, 16 MHz) */
static int getc_timeout(uint32_t ms)
{
    uint32_t t0 = DWT_CYCCNT;
    for (;;) {
        int c = uart2_getc();
        if (c >= 0) return c;
        if (DWT_CYCCNT - t0 > ms * 16000u) return -1;
    }
}

/*
 * 받기 규약 (tools/send_update.py)
 *   앱 → "READY\n"        PC → 길이 4바이트 (리틀 엔디언)
 *   앱 → "ERASED\n"       (다른 슬롯을 지운 뒤)
 *   PC → 256바이트 묶음    앱 → '#' (Flash에 쓴 뒤), 마지막 묶음은 짧을 수 있다
 *   앱 → "OK ...\n" 또는 "ERROR ...\n"
 */
static void receive_update(void)
{
    static uint8_t buf[CHUNK];
    int target = running_slot() ^ 1;               /* 지금 실행 중인 슬롯은 건드리지 않는다 */
    uint32_t base = slot_addr(target);

    puts_("READY\n");
    uint32_t len = 0;
    for (int i = 0; i < 4; i++) {
        int c = getc_timeout(5000);
        if (c < 0) { puts_("ERROR timeout\n"); return; }
        len |= (uint32_t)c << (8 * i);
    }
    if (len < IMG_HDR_SIZE + 8u || len > SLOT_SIZE) {
        puts_("ERROR bad length\n");
        return;
    }
    wdg_kick();
    if (flash_erase(base) != 0) {                  /* 1~2초: 워치독 4초 안 */
        puts_("ERROR erase\n");
        return;
    }
    wdg_kick();
    puts_("ERASED\n");

    for (uint32_t off = 0; off < len; off += CHUNK) {
        uint32_t n = (len - off < CHUNK) ? len - off : CHUNK;
        for (uint32_t i = 0; i < n; i++) {
            int c = getc_timeout(2000);
            if (c < 0) { puts_("ERROR timeout\n"); return; }
            buf[i] = (uint8_t)c;
        }
        uint32_t padded = (n + 3u) & ~3u;          /* 4바이트 단위로 쓴다: 남는 칸은 0xFF(지운 상태) */
        for (uint32_t i = n; i < padded; i++) buf[i] = 0xFF;
        if (flash_write(base + off, buf, padded) != 0) {
            puts_("ERROR write\n");
            return;
        }
        wdg_kick();
        uart2_putc('#');
    }

    image_hdr_t h;
    img_result_t r = image_check(target, &h);      /* 받은 것을 Flash에서 다시 읽어 검증 */
    if (r != IMG_OK) {
        puts_("\nERROR image: "); puts_(image_result_str(r)); puts_("\n");
        return;
    }
    boot_state_t st;
    meta_load(&st);
    boot_request(&st, target);
    if (meta_save(&st) != 0) {
        puts_("\nERROR boot state\n");
        return;
    }
    puts_("\nOK v"); print_version(h.version); puts_(" in slot "); uart2_putc(target ? 'B' : 'A');
    puts_(", press r to reboot and try it\n");
}

int main(void)
{
    uart2_init(115200, 16000000u);
    cycle_counter_init();
    puts_("\n");
    status();

    for (;;) {
        wdg_kick();
        int c = uart2_getc();
        switch (c) {
        case 's': status(); break;
        case 'c': {
            boot_state_t st;
            meta_load(&st);
            if (boot_confirm(&st, running_slot()) && meta_save(&st) == 0) {
                puts_("confirmed\n");
            } else {
                puts_("already confirmed\n");
            }
            break;
        }
        case 'u': receive_update(); break;
        case 'x':
            puts_("hanging... watchdog will reset\n");
            for (;;) {                             /* 워치독을 갱신하지 않는다 */
            }
        case 'r':
            puts_("reset\n");
            while ((USART2->SR & USART_SR_TC) == 0) {
            }
            SCB_AIRCR = SCB_AIRCR_SYSRESET;
            for (;;) {
            }
        default:
            break;
        }
    }
}
