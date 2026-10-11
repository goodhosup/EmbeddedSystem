/*
 * USART0 송신만, 9600 bps 8N1, 폴링 방식
 * 원리와 수신, 인터럽트, 링 버퍼는 14장에서 다룬다.
 *
 * UBRR0 = F_CPU / (16 × 9600) − 1 = 103 (오차 0.2%)
 * 레지스터: ATmega328P 데이터시트 USART0 장
 */
#include "uart.h"

#include <avr/io.h>

void uart_init(void)
{
    UBRR0  = 103;
    UCSR0A = 0;
    UCSR0B = (1 << TXEN0);                       /* 송신만 켜기 */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);      /* 8비트, 패리티 없음, 정지 1비트 */
}

void uart_putc(char c)
{
    while ((UCSR0A & (1 << UDRE0)) == 0) {       /* 송신 버퍼가 빌 때까지 */
    }
    UDR0 = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_put_fixed1(int16_t value10)
{
    char buf[8];
    uint8_t i = 0;
    uint16_t v;

    if (value10 < 0) {
        uart_putc('-');
        v = (uint16_t)(-(int32_t)value10);
    } else {
        v = (uint16_t)value10;
    }

    buf[i++] = (char)('0' + v % 10);             /* 소수 첫째 자리 */
    v /= 10;
    buf[i++] = '.';
    do {
        buf[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v > 0);

    while (i > 0) {                              /* 거꾸로 쌓았으므로 뒤에서부터 */
        uart_putc(buf[--i]);
    }
}
