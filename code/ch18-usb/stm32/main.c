/*
 * 18장 실습: STM32를 USB 가상 시리얼(CDC ACM) 장치로 만들기 (NUCLEO-F411RE + TinyUSB)
 *
 * 연결: NUCLEO의 USB 커넥터는 ST-LINK용이다. STM32 자신의 USB는 PA11(D−), PA12(D+)를
 * 따로 USB 케이블에 연결해야 한다(18.8절). GND도 연결한다.
 *
 * 동작: PC에서 새 시리얼 포트가 생긴다. 보낸 글자를 대문자로 바꿔 돌려준다.
 *       LD2는 USB가 연결되면 1초, 아니면 0.25초 간격으로 깜빡인다.
 */
#include <ctype.h>

#include "stm32f411_regs.h"
#include "tusb.h"

void clock_init_96mhz_hse(void);

#define OTG_FS_GCCFG (*(volatile uint32_t *)0x50000038u)   /* RM0383 22.16절 OTG_FS_GCCFG */
#define GCCFG_VBUSASEN   (1u << 18)
#define GCCFG_VBUSBSEN   (1u << 19)
#define GCCFG_NOVBUSSENS (1u << 21)

static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

uint32_t tusb_time_millis_api(void)      /* 새 버전 TinyUSB가 시간 확인에 쓴다 */
{
    return ms_ticks;
}

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);
}

static void usb_hw_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)RCC->AHB2ENR;

    gpio_set_af(GPIOA, 11, 10);                       /* PA11 = OTG_FS_DM (AF10) */
    gpio_set_af(GPIOA, 12, 10);                       /* PA12 = OTG_FS_DP */
    GPIOA->OSPEEDR |= (3u << (11 * 2)) | (3u << (12 * 2));

    /* VBUS 감지를 끈다: PA9에 VBUS를 연결하지 않았으므로 */
    OTG_FS_GCCFG = (OTG_FS_GCCFG & ~(GCCFG_VBUSASEN | GCCFG_VBUSBSEN)) | GCCFG_NOVBUSSENS;

    nvic_set_priority(OTG_FS_IRQn, 5);
}

int main(void)
{
    clock_init_96mhz_hse();

    SYSTICK->LOAD = 96000u - 1u;                      /* 96 MHz에서 1 ms */
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    GPIOA->MODER = (GPIOA->MODER & ~(3u << (5 * 2))) | (1u << (5 * 2));

    usb_hw_init();
    tud_init(BOARD_TUD_RHPORT);
    OTG_FS_GCCFG = (OTG_FS_GCCFG & ~(GCCFG_VBUSASEN | GCCFG_VBUSBSEN)) | GCCFG_NOVBUSSENS;

    uint32_t last = 0;
    for (;;) {
        tud_task();                                   /* USB 이벤트 처리: 자주 불러야 한다 */

        if (tud_cdc_available()) {
            uint8_t buf[64];
            uint32_t n = tud_cdc_read(buf, sizeof buf);
            for (uint32_t i = 0; i < n; i++) {
                buf[i] = (uint8_t)toupper(buf[i]);
            }
            tud_cdc_write(buf, n);
            tud_cdc_write_flush();
        }

        uint32_t period = tud_mounted() ? 1000u : 250u;
        if (ms_ticks - last >= period / 2u) {
            last = ms_ticks;
            GPIOA->ODR ^= (1u << 5);
        }
    }
}

/* 호스트가 포트를 열고 닫을 때(DTR) 불린다 */
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;
    if (dtr) {
        tud_cdc_write_str("\r\nch18 USB CDC: type something\r\n");
        tud_cdc_write_flush();
    }
}
