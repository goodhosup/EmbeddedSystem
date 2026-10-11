/*
 * 1 ms 틱: Timer0 CTC, 16 MHz / 64 / 250 = 1 kHz (8장)
 */
#include "tick.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uint32_t ms_ticks;

ISR(TIMER0_COMPA_vect)
{
    ms_ticks++;
}

void tick_init(void)
{
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);
}

uint32_t millis(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = ms_ticks;
    }
    return t;
}
