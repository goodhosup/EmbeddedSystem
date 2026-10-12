/*
 * 30장: USART2 (PA2 TX, PA3 RX, AF7)
 *   송신: DMA1 스트림 6, 채널 4 (RM0383 표 27 "DMA1 request mapping"의 USART2_TX)
 *         CPU는 DMA를 시작하고 완료 인터럽트를 세마포어로 기다린다 → 보내는 동안 다른 태스크가 실행
 *   수신: RXNE 인터럽트 → 큐 (27장과 같음)
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "uart_dma.h"
#include "semphr.h"
#include "stm32f411_regs.h"

#define DMA1_Stream6_IRQn 17u            /* RM0383 10.2절 벡터 표 위치 17 */
#define TX_STREAM 6u
#define TX_BUF    160u

static QueueHandle_t     rx_q;
static SemaphoreHandle_t tx_lock, tx_done;
static char tx_buf[TX_BUF];              /* DMA가 읽는 동안 내용이 그대로 있어야 하므로 정적 */

void USART2_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        char c = (char)USART2->DR;
        (void)xQueueSendFromISR(rx_q, &c, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

void DMA1_Stream6_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t shift = dma_flag_shift(TX_STREAM);
    uint32_t isr = DMA1->HISR >> shift;
    DMA1->HIFCR = DMA_ALL_FLAGS0 << shift;          /* 플래그 지우기 */
    if (isr & (DMA_TCIF0 | DMA_TEIF0)) {
        xSemaphoreGiveFromISR(tx_done, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

void uart_dma_init(uint32_t baud, uint32_t pclk1_hz)
{
    rx_q    = xQueueCreate(32, sizeof(char));
    tx_lock = xSemaphoreCreateMutex();
    tx_done = xSemaphoreCreateBinary();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;
    gpio_set_af(GPIOA, 2, 7);
    gpio_set_af(GPIOA, 3, 7);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << (3 * 2))) | (1u << (3 * 2));

    USART2->CR1 = 0;
    USART2->BRR = (pclk1_hz + baud / 2u) / baud;
    USART2->CR3 = USART_CR3_DMAT;                    /* TXE가 되면 DMA에 요청 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    DMA_Stream_TypeDef *st = &DMA1->S[TX_STREAM];
    st->CR  = 0;
    st->PAR = (uint32_t)&USART2->DR;
    st->FCR = 0;                                     /* 직접 모드 (FIFO 없음) */

    nvic_set_priority(USART2_IRQn, 6);               /* FromISR API를 부르므로 5~15 */
    nvic_enable_irq(USART2_IRQn);
    nvic_set_priority(DMA1_Stream6_IRQn, 6);
    nvic_enable_irq(DMA1_Stream6_IRQn);
}

static void dma_send(size_t n)
{
    DMA_Stream_TypeDef *st = &DMA1->S[TX_STREAM];
    st->CR &= ~DMA_SxCR_EN;
    while (st->CR & DMA_SxCR_EN) {                   /* 이전 전송이 완전히 멈출 때까지 */
    }
    DMA1->HIFCR = DMA_ALL_FLAGS0 << dma_flag_shift(TX_STREAM);
    st->M0AR = (uint32_t)tx_buf;
    st->NDTR = (uint32_t)n;
    st->CR   = DMA_SxCR_CHSEL(4) | DMA_SxCR_MINC | DMA_SxCR_DIR_M2P
             | DMA_SxCR_PSIZE_8 | DMA_SxCR_MSIZE_8 | DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    st->CR  |= DMA_SxCR_EN;
    (void)xSemaphoreTake(tx_done, pdMS_TO_TICKS(100));   /* 160 B @115200 ≈ 14 ms */
}

void uart_dma_write(const char *s, size_t n)
{
    xSemaphoreTake(tx_lock, portMAX_DELAY);          /* 버퍼와 DMA 스트림은 공유 자원 */
    while (n > 0) {
        size_t k = (n < TX_BUF) ? n : TX_BUF;
        memcpy(tx_buf, s, k);
        dma_send(k);
        s += k;
        n -= k;
    }
    xSemaphoreGive(tx_lock);
}

void log_printf(const char *fmt, ...)
{
    char line[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n > 0) {
        uart_dma_write(line, (size_t)n < sizeof line ? (size_t)n : sizeof line - 1u);
    }
}

QueueHandle_t uart_rx_queue(void)
{
    return rx_q;
}

void uart_panic_puts(const char *s)
{
    /* DMA 스트림이 멈춰 있으면 DR에 직접 써도 된다 (TXE의 DMA 요청은 꺼진 스트림이 무시) */
    while (*s) {
        while ((USART2->SR & USART_SR_TXE) == 0) {
        }
        USART2->DR = (uint8_t)*s++;
    }
}
