/*
 * TMP36 (A0 = ADC0), 내부 1.1 V 기준 (9장)
 *
 * TMP36: Vout = 500 mV + 10 mV/°C  →  °C × 10 = Vout(mV) − 500
 * 1.1 V 기준이면 1 LSB ≈ 1.07 mV ≈ 0.1 °C, 측정 상한은 약 60 °C
 *
 * 내부 기준 전압은 칩마다 1.0~1.2 V로 다르다(ATmega328P 데이터시트 VBG).
 * 우노의 AREF 핀 전압을 멀티미터로 재서 VREF_MV에 넣으면 정확해진다.
 */
#include "sensor.h"

#include <avr/io.h>

#ifndef VREF_MV
#define VREF_MV 1100u
#endif

#define SAMPLES 16u

static uint16_t adc_read(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
    }
    return ADC;
}

void sensor_init(void)
{
    ADMUX  = (1 << REFS1) | (1 << REFS0);                   /* 내부 1.1 V 기준, 채널 0 */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    DIDR0  = (1 << ADC0D);

    /* 기준 전압을 바꾼 직후 몇 번은 버린다 (AREF 커패시터가 새 전압으로 충전되는 시간) */
    for (uint8_t i = 0; i < 8; i++) {
        (void)adc_read();
    }
}

int16_t sensor_read_c10(void)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++) {
        sum += adc_read();
    }
    /* mV = 평균 × VREF / 1024. 16번 더한 값을 그대로 써서 반올림 오차를 줄인다 */
    uint32_t mv = (sum * VREF_MV + (SAMPLES * 1024u) / 2) / (SAMPLES * 1024u);
    return (int16_t)((int32_t)mv - 500);
}
