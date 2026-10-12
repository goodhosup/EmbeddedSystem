/*
 * 15장 실습: I2C 버스 스캔과 BMP280/BME280 온도 (아두이노 우노)
 *
 * 연결 (3.3 V 센서 모듈은 레벨 변환이 들어 있는지 확인. 15.7절)
 *   센서 VCC → 3.3 V 또는 5 V(모듈 사양), GND → GND, SDA → A4, SCL → A5
 * 시리얼: 115200 bps
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "i2c.h"
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
    i2c_init(100000);
    sei();

    app_run();
}
