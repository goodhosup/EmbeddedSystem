/*
 * 11장 실습: 파워다운 슬립과 워치독·핀 변화 깨우기 (ATmega328P)
 *
 * 동작
 *   - 워치독 인터럽트로 약 1초마다 깨어나 LED(D13)를 5 ms 켰다 끄고 다시 잔다.
 *   - D2 버튼(PCINT18)을 누르면 즉시 깨어나 LED를 200 ms 켠다.
 *
 * 우노 보드 전체의 전류는 USB 변환 칩, 레귤레이터, 전원 LED 때문에 수십 mA로 거의 줄지 않는다.
 * 칩만의 전류를 보려면 11.6절처럼 브레드보드에 ATmega328P를 따로 꽂아 잰다.
 *
 * 레지스터: ATmega328P 데이터시트
 *   Power Management and Sleep Modes 장 (SMCR, PRR, MCUCR의 BODS/BODSE)
 *   System Control and Reset 장의 Watchdog Timer (WDTCSR)
 *   External Interrupts 장의 Pin Change Interrupt (PCICR, PCMSK2)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

static volatile uint8_t wake_by_button;

ISR(WDT_vect)
{
    /* 깨우기만 한다. 인터럽트 모드라 리셋은 일어나지 않는다 */
}

ISR(PCINT2_vect)
{
    if ((PIND & (1 << PIND2)) == 0) {       /* 눌림 */
        wake_by_button = 1;
    }
}

static void watchdog_interrupt_1s(void)
{
    cli();
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    WDTCSR = (1 << WDCE) | (1 << WDE);                      /* 1) 변경 허가 (4클럭 안에 다음 쓰기) */
    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1);       /* 2) 인터럽트 모드, 약 1 s */
    sei();
}

static void low_power_setup(void)
{
    ADCSRA = 0;                     /* ADC 끄기 (켜져 있으면 슬립 중에도 전류를 쓴다) */
    ACSR  = (1 << ACD);             /* 아날로그 비교기 끄기 */
    PRR   = 0xFF;                   /* 쓰지 않는 주변장치 클럭 모두 끄기 */

    /* 쓰지 않는 핀은 풀업 켠 입력으로: 떠 있는 입력은 전류를 새게 한다 */
    DDRB = (1 << DDB5);  PORTB = (uint8_t)~(1 << PORTB5);
    DDRC = 0;            PORTC = 0xFF;
    DDRD = 0;            PORTD = 0xFF;   /* D2 버튼 풀업 포함 */
}

static void sleep_power_down(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sleep_bod_disable();            /* 슬립 동안 BOD 끄기 (시간 제한 있는 시퀀스) */
    sei();                          /* SEI 다음 명령까지는 인터럽트가 걸리지 않는다 */
    sleep_cpu();                    /* 여기서 잠들고, 깨어나면 ISR을 실행한 뒤 이어서 진행 */
    sleep_disable();
}

int main(void)
{
    low_power_setup();

    PCICR  |= (1 << PCIE2);         /* 포트 D 핀 변화 인터럽트 */
    PCMSK2 |= (1 << PCINT18);       /* PD2만 */

    watchdog_interrupt_1s();

    for (;;) {
        PORTB |= (1 << PORTB5);
        if (wake_by_button) {
            wake_by_button = 0;
            _delay_ms(200);
        } else {
            _delay_ms(5);
        }
        PORTB &= ~(1 << PORTB5);

        sleep_power_down();
    }
}
