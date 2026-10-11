/*
 * 12장 프로젝트: 디지털 온도계 (아두이노 우노)
 *
 * 요구사항 (12.2절)
 *   R1 1초마다 온도를 재서 PC로 출력한다 (0.1 °C 단위)
 *   R2 버튼을 짧게 누르면 °C와 °F 표시를 바꾼다
 *   R3 버튼을 1초 이상 누르면 최저/최고 기록을 지운다
 *   R4 30.0 °C 이상이면 경보 LED가 깜빡이고, 29.5 °C 아래로 내려가면 끈다
 *   R5 동작 중임을 보드 LED의 1초 깜빡임으로 보여 준다
 *
 * 연결: 그림 12-1
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "button.h"
#include "sensor.h"
#include "tick.h"
#include "uart.h"

#define ALARM_ON_C10    300     /* 30.0 °C */
#define ALARM_OFF_C10   295     /* 29.5 °C */

typedef enum { STATE_NORMAL, STATE_ALARM } state_t;

static state_t state = STATE_NORMAL;
static uint8_t show_fahrenheit;
static int16_t min_c10 = INT16_MAX, max_c10 = INT16_MIN;

static int16_t c10_to_f10(int16_t c10)
{
    return (int16_t)((int32_t)c10 * 9 / 5 + 320);
}

static void print_temp(const char *label, int16_t c10)
{
    uart_puts(label);
    if (show_fahrenheit) {
        uart_put_fixed1(c10_to_f10(c10));
        uart_puts(" F");
    } else {
        uart_put_fixed1(c10);
        uart_puts(" C");
    }
}

static void report(int16_t c10)
{
    print_temp("T=", c10);
    print_temp("  min=", min_c10);
    print_temp("  max=", max_c10);
    uart_puts(state == STATE_ALARM ? "  [ALARM]\r\n" : "\r\n");
}

static void update_state(int16_t c10)
{
    switch (state) {
    case STATE_NORMAL:
        if (c10 >= ALARM_ON_C10) {
            state = STATE_ALARM;
        }
        break;
    case STATE_ALARM:
        if (c10 < ALARM_OFF_C10) {
            state = STATE_NORMAL;
            PORTB &= ~(1 << PORTB0);            /* 경보 LED 끄기 */
        }
        break;
    }
}

int main(void)
{
    DDRB |= (1 << DDB5) | (1 << DDB0);          /* D13 상태 LED, D8 경보 LED */

    tick_init();
    button_init();
    sensor_init();
    uart_init();
    sei();

    uart_puts("\r\nDigital thermometer (ch12)\r\n");

    uint32_t last_button = 0, last_measure = 0, last_blink = 0;

    for (;;) {
        uint32_t now = millis();

        /* 5 ms마다: 버튼 */
        if (now - last_button >= 5) {
            last_button = now;
            switch (button_poll(now)) {
            case BUTTON_SHORT:
                show_fahrenheit = !show_fahrenheit;
                break;
            case BUTTON_LONG:
                min_c10 = INT16_MAX;
                max_c10 = INT16_MIN;
                uart_puts("min/max reset\r\n");
                break;
            default:
                break;
            }
        }

        /* 1초마다: 측정, 상태 갱신, 출력 */
        if (now - last_measure >= 1000) {
            last_measure = now;
            int16_t c10 = sensor_read_c10();
            if (c10 < min_c10) min_c10 = c10;
            if (c10 > max_c10) max_c10 = c10;
            update_state(c10);
            report(c10);
            PINB = (1 << PINB5);                /* 상태 LED 반전 */
        }

        /* 250 ms마다: 경보 LED 깜빡임 */
        if (now - last_blink >= 250) {
            last_blink = now;
            if (state == STATE_ALARM) {
                PINB = (1 << PINB0);
            }
        }
    }
}
