/*
 * 8장 실습: 타이머로 1 ms 틱, 디바운싱, PWM 밝기, 서보 (아두이노 우노)
 *
 * 연결
 *   D2  (PD2): 버튼 -- GND (내부 풀업)
 *   D11 (PB3 = OC2A): 330 Ω -- LED -- GND        (Timer2 고속 PWM, 약 977 Hz)
 *   D9  (PB1 = OC1A): 서보 신호선 (서보 전원은 별도 5 V, GND 공통)   (Timer1, 50 Hz)
 *
 * 버튼을 누를 때마다 LED 밝기와 서보 각도가 다음 단계로 바뀐다.
 *
 * 레지스터: ATmega328P 데이터시트
 *   8-bit Timer/Counter0 (CTC 모드), 16-bit Timer/Counter1 (모드 14: 고속 PWM, TOP = ICR1),
 *   8-bit Timer/Counter2 (모드 3: 고속 PWM) 장의 Register Description
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

static volatile uint32_t ms_ticks;

ISR(TIMER0_COMPA_vect)          /* 1 ms마다 */
{
    ms_ticks++;
}

static uint32_t millis(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = ms_ticks;
    }
    return t;
}

static void tick_init(void)
{
    /* Timer0 CTC: 16 MHz / 64 / (249 + 1) = 1000 Hz */
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);     /* ÷64 */
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);
}

static void led_pwm_init(void)
{
    /* Timer2 고속 PWM(모드 3), OC2A 비반전 출력, 16 MHz / 64 / 256 ≈ 977 Hz */
    DDRB  |= (1 << DDB3);
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS22);                   /* Timer2에서 CS22만 1이면 ÷64 */
    OCR2A  = 0;
}

static void servo_init(void)
{
    /* Timer1 모드 14(고속 PWM, TOP = ICR1), ÷8 → 틱 0.5 µs, 40000틱 = 20 ms */
    DDRB  |= (1 << DDB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1   = 39999;
    OCR1A  = 3000;                          /* 1.5 ms: 가운데 */
}

/* 5 ms마다 불러서, 같은 값이 4번 연속(20 ms)이면 상태가 바뀐 것으로 인정한다 */
static uint8_t button_pressed_event(void)
{
    static uint8_t stable = 1, count = 0;
    uint8_t now = (PIND >> PIND2) & 1u;

    if (now != stable) {
        if (++count >= 4) {
            stable = now;
            count = 0;
            return stable == 0;             /* 1 → 0으로 확정된 순간만 이벤트 */
        }
    } else {
        count = 0;
    }
    return 0;
}

int main(void)
{
    static const uint8_t  led_steps[]   = { 0, 8, 32, 96, 255 };
    static const uint16_t servo_steps[] = { 2000, 2500, 3000, 3500, 4000 };   /* 1.0 ~ 2.0 ms */
    uint8_t step = 0;

    DDRD  &= ~(1 << DDD2);
    PORTD |=  (1 << PORTD2);

    tick_init();
    led_pwm_init();
    servo_init();
    sei();

    uint32_t last_sample = 0;

    for (;;) {
        uint32_t now = millis();

        if (now - last_sample >= 5) {       /* 뺄셈으로 비교하면 넘침에도 안전하다 */
            last_sample = now;
            if (button_pressed_event()) {
                step = (step + 1) % sizeof(led_steps);
                OCR2A = led_steps[step];
                OCR1A = servo_steps[step];  /* 16비트 레지스터: 컴파일러가 순서에 맞게 쓴다 */
            }
        }
        /* 여기서 다른 일을 할 수 있다. 기다리는 코드가 없다 */
    }
}
