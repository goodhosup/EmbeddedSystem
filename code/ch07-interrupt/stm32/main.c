/*
 * 7장 실습: 외부 인터럽트(EXTI)와 디바운싱 (NUCLEO-F411RE)
 *
 *   LD2: PA5 (High = 켜짐),  B1: PC13 (누르면 Low)
 *
 * 동작은 AVR 버전과 같다.
 *   B1 하강 에지 → EXTI 라인 13 → EXTI15_10_IRQHandler
 *   ISR은 플래그만 세우고 라인 13을 잠시 막는다.
 *   메인이 LED를 반전하고, 약 20 ms 뒤에 다시 허용한다.
 *
 * 레지스터
 *   SYSCFG_EXTICR4: RM0383 7.2.6절 (라인 13의 포트 선택, 비트 7:4)
 *   EXTI_IMR/FTSR/PR: RM0383 10.3절
 *   NVIC: PM0214 4.2절
 */
#include "stm32f411_regs.h"

#define LED_PIN     5u
#define BUTTON_PIN  13u

static volatile uint32_t button_event;
static volatile uint32_t press_count;   /* 32비트 정렬 변수의 읽기/쓰기는 Cortex-M에서 원자적 */

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & (1u << BUTTON_PIN)) {
        EXTI->PR   = 1u << BUTTON_PIN;      /* 대기 플래그 지우기 (1을 쓴다) */
        EXTI->IMR &= ~(1u << BUTTON_PIN);   /* 바운싱 동안 막기 */
        button_event = 1;
        press_count++;
    }
}

static void delay(volatile uint32_t count)
{
    while (count--) {
    }
}

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << (LED_PIN * 2))) | (1u << (LED_PIN * 2));
    GPIOC->MODER &= ~(3u << (BUTTON_PIN * 2));
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3u << (BUTTON_PIN * 2))) | (1u << (BUTTON_PIN * 2));
}

static void exti_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;                    /* SYSCFG 클럭 */
    (void)RCC->APB2ENR;

    /* EXTI 라인 13 ← 포트 C (EXTICR4의 비트 7:4 = 0010) */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFu << 4)) | (2u << 4);

    EXTI->FTSR |=  (1u << BUTTON_PIN);      /* 하강 에지 */
    EXTI->RTSR &= ~(1u << BUTTON_PIN);
    EXTI->PR    =  (1u << BUTTON_PIN);      /* 남아 있을 플래그 지우기 */
    EXTI->IMR  |=  (1u << BUTTON_PIN);      /* 라인 13 허용 */

    nvic_set_priority(EXTI15_10_IRQn, 5);
    nvic_enable_irq(EXTI15_10_IRQn);
}

int main(void)
{
    gpio_init();
    exti_init();
    /* Cortex-M은 리셋 직후 PRIMASK = 0이라 전체 인터럽트가 이미 허용되어 있다 */

    for (;;) {
        if (button_event) {
            button_event = 0;
            if (GPIOA->ODR & (1u << LED_PIN)) {
                GPIOA->BSRR = 1u << (LED_PIN + 16);
            } else {
                GPIOA->BSRR = 1u << LED_PIN;
            }

            delay(100000);                          /* 16 MHz에서 수십 ms (8장에서 타이머로 바꾼다) */
            EXTI->PR   = 1u << BUTTON_PIN;
            EXTI->IMR |= 1u << BUTTON_PIN;
        }

        if (press_count >= 10) {
            uint32_t s = irq_save();                /* 읽고-비교하고-쓰는 동안 ISR이 끼어들지 않게 */
            if (press_count >= 10) {
                press_count = 0;
                GPIOA->BSRR = 1u << (LED_PIN + 16);
            }
            irq_restore(s);
        }
    }
}
