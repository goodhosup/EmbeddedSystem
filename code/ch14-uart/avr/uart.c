/*
 * USART0 인터럽트 방식 (아두이노 우노)
 *
 * 수신: RX 완료 인터럽트(USART_RX_vect)가 받은 바이트를 수신 링 버퍼에 넣는다.
 * 송신: uart_putc()가 송신 링 버퍼에 넣고, 데이터 레지스터가 비었다는 인터럽트
 *       (USART_UDRE_vect)가 하나씩 꺼내 UDR0에 쓴다.
 *
 * 레지스터: ATmega328P 데이터시트 USART0 장 (UCSR0A/B/C, UBRR0, UDR0)
 */
#include "uart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#define RX_SIZE 64u                 /* 2의 거듭제곱이어야 한다 (& 로 나머지 계산) */
#define TX_SIZE 64u

static volatile uint8_t rx_buf[RX_SIZE], tx_buf[TX_SIZE];
static volatile uint8_t rx_head, rx_tail, tx_head, tx_tail;
static volatile uint8_t rx_overflow;

ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;        /* 오류 플래그는 UDR0을 읽기 전에 읽어야 한다 */
    uint8_t c = UDR0;
    if (status & ((1 << FE0) | (1 << DOR0) | (1 << UPE0))) {
        return;                     /* 깨진 바이트는 버린다 */
    }
    uint8_t next = (rx_head + 1u) & (RX_SIZE - 1u);
    if (next == rx_tail) {
        rx_overflow = 1;            /* 꽉 참: 새 바이트를 버린다 */
        return;
    }
    rx_buf[rx_head] = c;
    rx_head = next;
}

ISR(USART_UDRE_vect)
{
    if (tx_head == tx_tail) {
        UCSR0B &= ~(1 << UDRIE0);   /* 보낼 것이 없으면 인터럽트 끄기 */
        return;
    }
    UDR0 = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1u) & (TX_SIZE - 1u);
}

void uart_init(uint32_t baud)
{
    /* 배속 모드(U2X0): UBRR = F_CPU / (8 × baud) − 1, 반올림 */
    uint16_t ubrr = (uint16_t)((F_CPU + 4u * baud) / (8u * baud) - 1u);
    UBRR0  = ubrr;
    UCSR0A = (1 << U2X0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);                  /* 8N1 */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);    /* 수신·송신, 수신 인터럽트 */
}

void uart_putc(char c)
{
    uint8_t next = (tx_head + 1u) & (TX_SIZE - 1u);
    while (next == tx_tail) {       /* 송신 버퍼가 꽉 차면 ISR이 비울 때까지 기다린다 */
    }
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    UCSR0B |= (1 << UDRIE0);        /* 송신 인터럽트 켜기 */
}

int uart_getc(void)
{
    if (rx_head == rx_tail) {
        return -1;
    }
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_SIZE - 1u);
    return c;
}

uint8_t uart_rx_overflow(void)
{
    uint8_t o = rx_overflow;
    rx_overflow = 0;
    return o;
}

/* ---- printf 연결 (avr-libc의 stdio) ---- */
static int put_for_stdio(char c, FILE *f)
{
    (void)f;
    if (c == '\n') {
        uart_putc('\r');            /* 터미널 줄바꿈: \r\n */
    }
    uart_putc(c);
    return 0;
}

static FILE uart_stdout = FDEV_SETUP_STREAM(put_for_stdio, NULL, _FDEV_SETUP_WRITE);

void uart_stdio_init(void)
{
    stdout = &uart_stdout;
}
