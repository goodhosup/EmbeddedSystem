/*
 * 9장 실습: 가변저항으로 LED 밝기 조절 (아두이노 우노)
 *
 * 연결
 *   A0 (PC0 = ADC0): 10 kΩ 가변저항의 가운데 다리. 양쪽 다리는 5 V와 GND
 *   D11 (PB3 = OC2A): 330 Ω -- LED -- GND   (8장의 Timer2 PWM)
 *
 * 10 ms마다 ADC를 읽어 최근 8개를 평균하고, 그 값으로 PWM 듀티비를 정한다.
 *
 * 레지스터: ATmega328P 데이터시트, Analog-to-Digital Converter 장
 *   ADMUX  REFS1:0 = 01 (AVCC 기준), MUX3:0 = 채널
 *   ADCSRA ADEN, ADSC, ADPS2:0 = 111 (÷128 → 125 kHz)
 *   DIDR0  ADC0D (디지털 입력 끄기)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

#define AVG_N 8

static volatile uint32_t ms_ticks;
volatile uint16_t pot_raw, pot_avg;      /* 디버거나 7장 이후 UART로 확인용 */

ISR(TIMER0_COMPA_vect)
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

static void adc_init(void)
{
    ADMUX  = (1 << REFS0);                       /* 기준 = AVCC(5 V), 오른쪽 정렬, 채널 0 */
    ADCSRA = (1 << ADEN)                         /* ADC 켜기 */
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   /* 16 MHz ÷ 128 = 125 kHz */
    DIDR0  = (1 << ADC0D);                       /* A0의 디지털 입력 버퍼 끄기 */
}

static uint16_t adc_read(uint8_t ch)
{
    ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);        /* 채널 선택 */
    ADCSRA |= (1 << ADSC);                       /* 변환 시작 */
    while (ADCSRA & (1 << ADSC)) {               /* 끝나면 ADSC가 0이 된다 (약 104 µs) */
    }
    return ADC;                                  /* ADCL, ADCH를 순서대로 읽는 16비트 접근 */
}

int main(void)
{
    static uint16_t buf[AVG_N];
    uint8_t idx = 0;
    uint32_t sum = 0;

    /* 1 ms 틱 (8장) */
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);

    /* LED PWM (8장) */
    DDRB  |= (1 << DDB3);
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS22);

    adc_init();
    sei();

    uint32_t last = 0;
    for (;;) {
        uint32_t now = millis();
        if (now - last >= 10) {
            last = now;

            pot_raw = adc_read(0);
            sum -= buf[idx];                     /* 가장 오래된 값을 빼고 */
            buf[idx] = pot_raw;                  /* 새 값을 넣고 */
            sum += pot_raw;                      /* 더한다: 매번 8개를 다시 더할 필요가 없다 */
            idx = (idx + 1) % AVG_N;
            pot_avg = (uint16_t)(sum / AVG_N);

            OCR2A = (uint8_t)(pot_avg >> 2);     /* 10비트(0~1023) → 8비트(0~255) */
        }
    }
}
