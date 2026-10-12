/*
 * 32장: STM32F411 부트로더 (섹터 0~1, 32 KB)
 *
 *   1. 부트 상태(bootmeta)와 두 슬롯의 이미지(머리말, CRC, 벡터)를 확인
 *   2. bootlogic의 규칙으로 부팅할 슬롯을 정하고, 상태가 바뀌었으면 먼저 저장
 *   3. 독립 워치독을 켜고(약 4초) 앱으로 점프: 새 앱이 멈추면 리셋되어 다시 여기로 온다
 * 실행할 이미지가 없으면 LD2를 빠르게 깜박이며 기다린다.
 */
#include "stm32f411_regs.h"
#include "uart2.h"
#include "layout.h"
#include "image.h"
#include "bootmeta.h"
#include "bootlogic.h"

static void puts_(const char *s)
{
    while (*s) {
        if (*s == '\n') uart2_putc('\r');
        uart2_putc(*s++);
    }
}

static void put_hex(uint32_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart2_putc(hx[(v >> i) & 0xFu]);
    }
}

static void iwdg_start(void)
{
    /* LSI 약 32 kHz / 64 = 500 Hz, 2000 카운트 → 약 4초. LSI는 칩마다 17~47 kHz라 실제로는 2.7~7.5초 */
    IWDG->KR  = IWDG_KEY_START;            /* 시작하면 리셋 전까지 멈출 수 없다 */
    IWDG->KR  = IWDG_KEY_ACCESS;
    IWDG->PR  = 4u;                        /* /64 */
    IWDG->RLR = 2000u;
    while (IWDG->SR != 0) {
    }
    IWDG->KR  = IWDG_KEY_RELOAD;
}

/* 앱으로 점프: 부트로더가 켠 것을 모두 되돌려 놓아야 앱이 리셋 직후와 같은 상태에서 시작한다 */
static void jump_to_app(uint32_t vt_addr)
{
    const uint32_t *vt = (const uint32_t *)vt_addr;

    __asm volatile ("cpsid i" ::: "memory");
    SYSTICK->CTRL = 0;
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;       /* 모든 인터럽트 금지 */
        NVIC->ICPR[i] = 0xFFFFFFFFu;       /* 대기 중인 것도 지움 */
    }
    RCC->APB1RSTR |= RCC_APB1ENR_USART2EN; /* USART2를 리셋 상태로 (리셋 비트 위치가 클럭 비트와 같다) */
    RCC->APB1RSTR &= ~RCC_APB1ENR_USART2EN;
    RCC->APB1ENR  &= ~RCC_APB1ENR_USART2EN;

    SCB_VTOR = vt_addr;                    /* 이제부터 예외는 앱의 벡터 테이블로 */
    __asm volatile ("dsb\n isb" ::: "memory");

    /* 앱의 초기 SP를 MSP에 넣고, 리셋 처리기로 점프. 인터럽트는 리셋 직후처럼 허용 상태로 */
    __asm volatile (
        "msr msp, %0 \n"
        "cpsie i     \n"
        "bx  %1      \n"
        :: "r" (vt[0]), "r" (vt[1]) : "memory");
    for (;;) {
    }
}

static void no_image_loop(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << 10)) | (1u << 10);
    for (;;) {
        GPIOA->ODR ^= 1u << 5;
        for (volatile uint32_t i = 0; i < 200000u; i++) {
        }
    }
}

int main(void)
{
    uart2_init(115200, 16000000u);
    puts_("\n[boot] ch32 bootloader\n");

    boot_state_t st;
    if (meta_load(&st) != 0) {
        puts_("[boot] no boot state, default: slot A\n");
    }
    bool valid[2];
    for (int s = 0; s < 2; s++) {
        image_hdr_t h;
        img_result_t r = image_check(s, &h);
        valid[s] = (r == IMG_OK);
        puts_(s == 0 ? "[boot] slot A: " : "[boot] slot B: ");
        puts_(image_result_str(r));
        if (valid[s]) {
            puts_(", version ");
            put_hex(h.version);
        }
        puts_("\n");
    }

    boot_decision_t d = boot_decide(&st, valid);
    puts_("[boot] decision: ");
    puts_(boot_event_str(d.event));
    puts_("\n");
    if (d.save && meta_save(&st) != 0) {
        puts_("[boot] WARNING: could not save boot state\n");
    }
    if (d.slot < 0) {
        no_image_loop();
    }

    puts_(d.slot == 0 ? "[boot] jumping to slot A\n" : "[boot] jumping to slot B\n");
    while ((USART2->SR & USART_SR_TC) == 0) {   /* 마지막 글자가 다 나갈 때까지 */
    }
    iwdg_start();
    jump_to_app(slot_addr(d.slot) + IMG_HDR_SIZE);
}
