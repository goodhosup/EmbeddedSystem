/*
 * 5장: avr-gcc로 직접 빌드하는 Blink (아두이노 우노, ATmega128 공용)
 *
 *   make flash                      우노 (부트로더)
 *   make flash BOARD=m128           ATmega128 (USBasp)
 *
 * LED 위치
 *   우노:      D13 = PB5 (보드 내장 LED)
 *   ATmega128: 키트마다 다르다. 여기서는 PB0에 LED가 있다고 가정한다.
 *              키트 회로도를 보고 LED_DDR, LED_PORT, LED_BIT를 바꾼다.
 */
#include <avr/io.h>
#include <util/delay.h>     /* _delay_ms(): F_CPU 값으로 지연 시간을 계산한다 */

#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__)
#define LED_DDR   DDRB
#define LED_PORT  PORTB
#define LED_BIT   PB0
#else
#define LED_DDR   DDRB
#define LED_PORT  PORTB
#define LED_BIT   PB5
#endif

int main(void)
{
    LED_DDR |= (1 << LED_BIT);

    for (;;) {
        LED_PORT ^= (1 << LED_BIT);
        _delay_ms(500);
    }
}
