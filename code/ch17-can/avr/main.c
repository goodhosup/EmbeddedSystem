/*
 * 17장 실습: MCP2515 CAN (아두이노 우노)
 * 연결: 그림 17-7. 모듈 VCC 5 V, SCK D13, SO D12, SI D11, CS D10
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
    spi_init(4000000);                  /* MCP2515: 최대 10 MHz */
    sei();

    app_run();
}
