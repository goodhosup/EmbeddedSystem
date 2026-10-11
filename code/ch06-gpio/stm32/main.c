/*
 * 6장 실습: 버튼을 누를 때마다 LED 반전 (NUCLEO-F411RE)
 *
 * 보드 연결 (NUCLEO-64 사용자 설명서 UM1724)
 *   LD2 (초록 LED): PA5, High = 켜짐
 *   B1  (파란 버튼): PC13, 누르면 Low
 *
 * make SPEED=1 로 빌드하면 PA5를 BSRR로 가능한 한 빨리 반전시킨다 (6.8절).
 *
 * 레지스터: RM0383 8.4절 GPIO registers
 */
#include "stm32f411_regs.h"

#define LED_PIN     5u      /* PA5 */
#define BUTTON_PIN  13u     /* PC13 */

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;     /* 클럭이 켜질 때까지 한 번 읽어 기다린다 (6.5절) */

    /* PA5: 범용 출력(01), 푸시풀, 저속, 풀업/풀다운 없음 */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (LED_PIN * 2))) | (1u << (LED_PIN * 2));
    GPIOA->OTYPER &= ~(1u << LED_PIN);
    GPIOA->OSPEEDR &= ~(3u << (LED_PIN * 2));
    GPIOA->PUPDR  &= ~(3u << (LED_PIN * 2));

    /* PC13: 입력(00), 풀업(01) */
    GPIOC->MODER &= ~(3u << (BUTTON_PIN * 2));
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3u << (BUTTON_PIN * 2))) | (1u << (BUTTON_PIN * 2));
}

static void led_toggle(void)
{
    if (GPIOA->ODR & (1u << LED_PIN)) {
        GPIOA->BSRR = 1u << (LED_PIN + 16);     /* BR5: Low */
    } else {
        GPIOA->BSRR = 1u << LED_PIN;            /* BS5: High */
    }
}

int main(void)
{
    gpio_init();

#ifdef SPEED_TEST
    for (;;) {
        GPIOA->BSRR = 1u << LED_PIN;
        GPIOA->BSRR = 1u << (LED_PIN + 16);
    }
#else
    uint32_t prev = 1;

    for (;;) {
        uint32_t now = (GPIOC->IDR >> BUTTON_PIN) & 1u;

        if (prev == 1 && now == 0) {    /* 누르는 순간 */
            led_toggle();
        }
        prev = now;
    }
#endif
}
