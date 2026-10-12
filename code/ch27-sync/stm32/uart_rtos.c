/*
 * 27장: USART2 (PA2 TX, PA3 RX, AF7)
 *   수신: RXNE 인터럽트 → xQueueSendFromISR로 글자를 큐에 → 태스크가 xQueueReceive로 기다림
 *   송신: 폴링 (printf는 print_lock 뮤텍스 안에서만 부른다)
 */
#include "uart_rtos.h"
#include "stm32f411_regs.h"

static QueueHandle_t rx_q;
static volatile uint32_t rx_dropped;

void USART2_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        char c = (char)USART2->DR;                     /* SR 다음 DR 읽기로 플래그가 지워진다 */
        if (xQueueSendFromISR(rx_q, &c, &woken) != pdPASS) {
            rx_dropped++;                              /* 큐가 가득 참: 기다리지 않고 버린다 */
        }
    }
    portYIELD_FROM_ISR(woken);     /* 더 높은 태스크가 깨어났으면 ISR이 끝나자마자 그 태스크로 */
}

void uart_rtos_init(uint32_t baud, uint32_t pclk1_hz)
{
    rx_q = xQueueCreate(32, sizeof(char));

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOA, 2, 7);
    gpio_set_af(GPIOA, 3, 7);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << (3 * 2))) | (1u << (3 * 2));

    USART2->CR1 = 0;
    USART2->BRR = (pclk1_hz + baud / 2u) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* FreeRTOS API를 부르는 ISR: configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY(5) 이상 숫자여야 한다 */
    nvic_set_priority(USART2_IRQn, 6);
    nvic_enable_irq(USART2_IRQn);
}

void uart_putc(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
    USART2->DR = (uint8_t)c;
}

QueueHandle_t uart_rx_queue(void)
{
    return rx_q;
}

int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart_putc('\r');
    }
    uart_putc((char)ch);
    return ch;
}
