/*
 * 3장 실습: 같은 지연 루프, 다른 클럭 (아두이노 우노, ATmega328P)
 *
 * 1) 16 MHz로 내장 LED(D13 = PB5)를 10번 깜빡인다.
 * 2) 시스템 클럭 프리스케일러로 클럭을 1/8(2 MHz)로 낮춘다.
 * 3) 같은 지연 루프로 계속 깜빡인다. 깜빡이는 속도가 8배 느려진다.
 *
 * CLKPR: ATmega328P 데이터시트, System Clock and Clock Options 장
 *        "System Clock Prescaler" 절의 CLKPR 레지스터 설명.
 * CLKPCE를 1로 쓴 뒤 4클럭 안에 CLKPS를 써야 한다.
 */
#include <avr/io.h>
#include <stdint.h>

static void delay(volatile uint32_t count)
{
    while (count--) {
    }
}

int main(void)
{
    DDRB |= (1 << DDB5);                 /* PB5(D13): 출력 */

    for (uint8_t i = 0; i < 20; i++) {   /* 16 MHz */
        PINB = (1 << PINB5);             /* PIN 레지스터에 1을 쓰면 출력이 반전된다 (6장) */
        delay(200000);
    }

    CLKPR = (1 << CLKPCE);               /* 1) 변경 허가 */
    CLKPR = (1 << CLKPS1) | (1 << CLKPS0);   /* 2) 4클럭 안에 ÷8 설정 (CLKPS = 0011) */

    for (;;) {                           /* 2 MHz: 같은 루프, 더 느린 깜빡임 */
        PINB = (1 << PINB5);
        delay(200000);
    }
}
