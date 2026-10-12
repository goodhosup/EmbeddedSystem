/*
 * 40장: USART1 (PA9 = TX = D8, PA10 = RX = D2, AF7) — ESP32와의 링크
 *   수신: RXNE 인터럽트 → 링 버퍼 (14장), 송신: 폴링
 */
#include "stm32f411_regs.h"
#include "link_uart.h"

#define RX_SIZE 512u
#define USART1_IRQn 37u

static volatile uint8_t  rx_buf[RX_SIZE];
static volatile uint32_t rx_head, rx_tail, rx_overflow;

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t c = (uint8_t)USART1->DR;
        uint32_t next = (rx_head + 1u) & (RX_SIZE - 1u);
        if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; } else { rx_overflow++; }
    }
}

void link_uart_init(uint32_t baud, uint32_t pclk2_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;
    gpio_set_af(GPIOA, 9, 7);
    gpio_set_af(GPIOA, 10, 7);
    USART1->CR1 = 0;
    USART1->BRR = (pclk2_hz + baud / 2u) / baud;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;
    nvic_set_priority(USART1_IRQn, 6);
    nvic_enable_irq(USART1_IRQn);
}

void link_uart_write(const uint8_t *p, size_t n)
{
    while (n--) {
        while ((USART1->SR & USART_SR_TXE) == 0) {
        }
        USART1->DR = *p++;
    }
}

int link_uart_getc(void)
{
    if (rx_head == rx_tail) return -1;
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_SIZE - 1u);
    return c;
}
