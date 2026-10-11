/*
 * 6장 실습: 버튼을 누를 때마다 LED 반전 (아두이노 우노, ATmega328P)
 *
 * 연결
 *   D13 (PB5): 보드 내장 LED
 *   D2  (PD2): 택트 스위치 -- GND (내부 풀업, 누르면 Low)
 *
 * make SPEED=1 로 빌드하면 PB5를 가능한 한 빨리 반전시킨다.
 * 오실로스코프나 로직 애널라이저로 D13의 주파수를 재 본다 (6.8절).
 *
 * 레지스터: ATmega328P 데이터시트, I/O-Ports 장의 Register Description
 *   PINx에 1을 쓰면 PORTx의 해당 비트가 반전된다 ("Toggling the Pin" 절)
 */
#include <avr/io.h>
#include <stdint.h>

#define LED_BIT     PB5
#define BUTTON_BIT  PD2

#ifdef SPEED_TEST
int main(void)
{
    DDRB |= (1 << LED_BIT);
    for (;;) {
        PINB = (1 << LED_BIT);      /* SBI 명령 하나로 반전 */
    }
}
#else
int main(void)
{
    DDRB  |=  (1 << LED_BIT);       /* PB5: 출력 */
    DDRD  &= ~(1 << BUTTON_BIT);    /* PD2: 입력 */
    PORTD |=  (1 << BUTTON_BIT);    /* PD2: 내부 풀업 */

    uint8_t prev = 1;               /* 풀업이므로 평소 값은 1 */

    for (;;) {
        uint8_t now = (PIND >> BUTTON_BIT) & 1u;

        if (prev == 1 && now == 0) {        /* 1 → 0: 누르는 순간(하강 에지) */
            PINB = (1 << LED_BIT);          /* LED 반전 */
        }
        prev = now;
    }
}
#endif
