/*
 * 7장 실습: 외부 인터럽트와 디바운싱 (아두이노 우노, ATmega328P)
 *
 * 연결
 *   D13 (PB5): 보드 내장 LED
 *   D2  (PD2 = INT0): 택트 스위치 -- GND (내부 풀업, 누르면 Low)
 *
 * 동작
 *   버튼을 누르면 INT0 하강 에지 인터럽트가 걸린다.
 *   ISR은 플래그만 세우고 INT0을 잠시 막는다.
 *   메인 루프가 LED를 반전하고, 20 ms 뒤에 INT0을 다시 허용한다.
 *
 * 레지스터: ATmega328P 데이터시트, External Interrupts 장
 *   EICRA (ISC01:ISC00 = 10 → 하강 에지), EIMSK (INT0), EIFR (INTF0, 1을 써서 지움)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>

static volatile uint8_t  button_event;   /* ISR → 메인: 버튼이 눌렸다 */
static volatile uint16_t press_count;    /* ISR이 늘리는 16비트 카운터 */

ISR(INT0_vect)
{
    EIMSK &= ~(1 << INT0);       /* 바운싱 동안 더 이상 받지 않는다 */
    button_event = 1;
    press_count++;
}

static uint16_t read_press_count(void)
{
    uint16_t n;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {  /* 8비트 두 번 읽는 동안 인터럽트 금지 */
        n = press_count;
    }
    return n;
}

int main(void)
{
    DDRB  |=  (1 << DDB5);
    DDRD  &= ~(1 << DDD2);
    PORTD |=  (1 << PORTD2);

    EICRA = (EICRA & ~((1 << ISC01) | (1 << ISC00))) | (1 << ISC01);   /* INT0: 하강 에지 */
    EIFR  = (1 << INTF0);        /* 혹시 남아 있을 플래그 지우기 */
    EIMSK |= (1 << INT0);        /* INT0 허용 */
    sei();                       /* 전체 인터럽트 허용 (SREG의 I 비트) */

    for (;;) {
        if (button_event) {
            button_event = 0;
            PINB = (1 << PINB5);                 /* LED 반전 */

            _delay_ms(20);                       /* 바운싱이 끝날 때까지 기다린다 */
            EIFR  = (1 << INTF0);                /* 그동안 생긴 플래그를 지우고 */
            EIMSK |= (1 << INT0);                /* 다시 허용 */
        }

        if (read_press_count() >= 10) {          /* 10번 누르면 LED를 끄고 처음부터 */
            PORTB &= ~(1 << PORTB5);
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                press_count = 0;
            }
        }
    }
}
