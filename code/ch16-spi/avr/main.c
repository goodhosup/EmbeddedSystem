/*
 * 16장 실습: SPI 플래시 (아두이노 우노)
 * 연결: 그림 16-1, 16.6절. 플래시는 3.3 V 전원과 레벨 변환 필요
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "spi.h"
#include "uart.h"

void app_run(void);

static volatile uint32_t ms_ticks;

ISR(TIMER0_COMPA_vect)
{
    ms_ticks++;
}

uint32_t board_millis(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = ms_ticks;
    }
    return t;
}

int main(void)
{
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);

    uart_init(115200);
    uart_stdio_init();
    spi_init(4000000);                  /* 4 MHz */
    sei();

    app_run();
}
