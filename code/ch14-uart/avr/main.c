/*
 * 14장 실습: UART 명령어 셸 (아두이노 우노)
 *
 * 시리얼 모니터(115200 bps, 줄 끝 "Newline" 또는 "Both NL & CR")에서 명령을 입력한다.
 *   help        명령 목록
 *   led on/off  D13 LED 켜기/끄기
 *   adc         A0의 ADC 값과 전압
 *   uptime      켜진 뒤 경과 시간
 *   echo 문자열  그대로 돌려주기
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>

#include "uart.h"

#define LINE_MAX 32

static volatile uint32_t ms_ticks;

ISR(TIMER0_COMPA_vect)
{
    ms_ticks++;
}

static uint32_t millis(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = ms_ticks;
    }
    return t;
}

static uint16_t adc_read0(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
    }
    return ADC;
}

static void run_command(char *line)
{
    char *cmd = strtok(line, " ");
    char *arg = strtok(NULL, "");

    if (cmd == NULL) {
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        printf("help, led on|off, adc, uptime, echo <text>\n");
    } else if (strcmp(cmd, "led") == 0 && arg != NULL && strcmp(arg, "on") == 0) {
        PORTB |= (1 << PORTB5);
        printf("LED on\n");
    } else if (strcmp(cmd, "led") == 0 && arg != NULL && strcmp(arg, "off") == 0) {
        PORTB &= ~(1 << PORTB5);
        printf("LED off\n");
    } else if (strcmp(cmd, "adc") == 0) {
        uint16_t v = adc_read0();
        uint32_t mv = (uint32_t)v * 5000u / 1024u;
        printf("A0 = %u (%lu mV)\n", v, (unsigned long)mv);
    } else if (strcmp(cmd, "uptime") == 0) {
        printf("%lu ms\n", (unsigned long)millis());
    } else if (strcmp(cmd, "echo") == 0) {
        printf("%s\n", arg ? arg : "");
    } else {
        printf("unknown command: %s (type help)\n", cmd);
    }
}

int main(void)
{
    char line[LINE_MAX];
    uint8_t len = 0;

    DDRB |= (1 << DDB5);

    TCCR0A = (1 << WGM01);                       /* 1 ms 틱 (8장) */
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);

    ADMUX  = (1 << REFS0);                       /* ADC (9장) */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    uart_init(115200);
    uart_stdio_init();
    sei();

    printf("\nch14 UART shell. type help\n> ");

    for (;;) {
        int c = uart_getc();
        if (c < 0) {
            continue;                            /* 받은 문자가 없으면 다른 일을 해도 된다 */
        }
        if (c == '\r' || c == '\n') {
            if (len > 0) {
                line[len] = '\0';
                printf("\n");
                run_command(line);
                len = 0;
            }
            printf("> ");
        } else if ((c == '\b' || c == 0x7F) && len > 0) {
            len--;                               /* 백스페이스 */
            printf("\b \b");
        } else if (c >= ' ' && len < LINE_MAX - 1) {
            line[len++] = (char)c;
            uart_putc((char)c);                  /* 입력한 글자를 되돌려 보여 준다 (에코) */
        }
        if (uart_rx_overflow()) {
            printf("\n[rx overflow]\n");
        }
    }
}
