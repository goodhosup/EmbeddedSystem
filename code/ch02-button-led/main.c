/*
 * 2장 실습: 버튼으로 LED 켜기 (아두이노 우노, ATmega328P)
 *
 * 연결
 *   D8 (PB0) -- 330 Ω -- LED 애노드, LED 캐소드 -- GND
 *   D2 (PD2) -- 택트 스위치 -- GND   (내부 풀업 사용, 누르면 Low)
 *
 * 레지스터 이름과 비트 위치: ATmega328P 데이터시트, I/O-Ports 장
 *   "Register Description" (DDRB/PORTB/PINB, DDRD/PORTD/PIND)
 */
#include <avr/io.h>

int main(void)
{
    DDRB  |=  (1 << DDB0);     /* PB0(D8): 출력 */
    DDRD  &= ~(1 << DDD2);     /* PD2(D2): 입력 */
    PORTD |=  (1 << PORTD2);   /* PD2 내부 풀업 켜기 */

    for (;;) {
        if ((PIND & (1 << PIND2)) == 0) {   /* 버튼 눌림 = Low */
            PORTB |=  (1 << PORTB0);        /* LED 켜기 */
        } else {
            PORTB &= ~(1 << PORTB0);        /* LED 끄기 */
        }
    }
}
