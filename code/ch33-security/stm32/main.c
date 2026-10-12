/*
 * 33장 실습: 보드에서 서명 검증하기 (NUCLEO-F411RE)
 *
 *   1. 슬롯 B(0x0804 0000)에 올려 둔 서명된 이미지(.simg)를 공개 키로 검증하고 걸린 시간을 잰다.
 *   2. Flash 읽기 보호(RDP) 수준을 옵션 바이트에서 읽어 보여 준다 (읽기만 한다. 바꾸지 않는다!)
 *
 * 공개 키는 ../keys/pubkey.h (make keys 로 만든다). 개인 키 ../keys/signing_key.pem은 저장소에 올리지 않는다.
 */
#include <stdio.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "secure_image.h"
#include "pubkey.h"

#define SLOT_B   0x08040000u
#define SLOT_SZ  (128u * 1024u)
#define MIN_COUNTER 2u           /* 실제 장치라면 OTP나 보호된 Flash에 기록된 값 */

static const char *rdp_level(void)
{
    uint32_t rdp = (FLASH->OPTCR >> 8) & 0xFFu;   /* RM0383 3.6절 옵션 바이트: RDP[15:8] */
    if (rdp == 0xAAu) return "level 0 (no protection: debugger can read Flash)";
    if (rdp == 0xCCu) return "level 2 (debug disabled PERMANENTLY)";
    return "level 1 (debugger cannot read Flash; going back to 0 erases it)";
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    cycle_counter_init();

    printf("\nch33 security demo\n");
    printf("RDP: %s\n", rdp_level());

    uint32_t t0 = DWT_CYCCNT;
    sv_result_t r = secure_verify((const uint8_t *)SLOT_B, SLOT_SZ, signing_pubkey, MIN_COUNTER);
    uint32_t ms = (DWT_CYCCNT - t0) / 16000u;
    const signed_hdr_t *h = (const signed_hdr_t *)SLOT_B;
    printf("slot B: %s", secure_result_str(r));
    if (r == SV_OK || r == SV_ROLLBACK) {
        printf(" (v%lu.%lu.%lu, counter %lu)", (unsigned long)(h->version >> 16),
               (unsigned long)((h->version >> 8) & 0xFF), (unsigned long)(h->version & 0xFF),
               (unsigned long)h->sec_counter);
    }
    printf("\nverify took %lu ms at 16 MHz\n", (unsigned long)ms);

    for (;;) {
        __asm volatile ("wfi");
    }
}
