/*
 * USART2 공통 모듈 (14장)
 *   PA2 = TX, PA3 = RX (AF7), 8N1, 수신은 RXNE 인터럽트 + 링 버퍼, 송신은 폴링
 *   __io_putchar()를 제공하므로 USE_NEWLIB = 1이면 printf가 여기로 나온다.
 */
#include "uart2.h"
#include "stm32f411_regs.h"

#define RX_SIZE 128u

static volatile uint8_t  rx_buf[RX_SIZE];
static volatile uint32_t rx_head, rx_tail;

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t c = (uint8_t)USART2->DR;
        if (sr & (USART_SR_FE | USART_SR_PE)) {
            return;
        }
        uint32_t next = (rx_head + 1u) & (RX_SIZE - 1u);
        if (next != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next;
        }
    }
}

void uart2_init(uint32_t baud, uint32_t pclk1_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    gpio_set_af(GPIOA, 2, 7);
    gpio_set_af(GPIOA, 3, 7);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << (3 * 2))) | (1u << (3 * 2));

    USART2->CR1 = 0;
    USART2->BRR = (pclk1_hz + baud / 2u) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    nvic_set_priority(USART2_IRQn, 6);
    nvic_enable_irq(USART2_IRQn);
}

void uart2_putc(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
    USART2->DR = (uint8_t)c;
}

int uart2_getc(void)
{
    if (rx_head == rx_tail) {
        return -1;
    }
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_SIZE - 1u);
    return c;
}

int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart2_putc('\r');
    }
    uart2_putc((char)ch);
    return ch;
}
