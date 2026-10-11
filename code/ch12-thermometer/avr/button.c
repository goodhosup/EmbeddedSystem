/*
 * 버튼 (D2 = PD2, GND 쪽, 내부 풀업)
 * 8장의 샘플링 디바운싱에 누른 시간 측정을 더했다.
 */
#include "button.h"

#include <avr/io.h>

#define STABLE_COUNT   4u       /* 5 ms × 4 = 20 ms */
#define LONG_PRESS_MS  1000u

static uint8_t  stable = 1;     /* 1 = 뗌, 0 = 눌림 */
static uint8_t  count;
static uint32_t pressed_at;
static uint8_t  long_reported;

void button_init(void)
{
    DDRD  &= ~(1 << DDD2);
    PORTD |=  (1 << PORTD2);
}

button_event_t button_poll(uint32_t now_ms)
{
    uint8_t raw = (PIND >> PIND2) & 1u;

    if (raw != stable) {
        if (++count >= STABLE_COUNT) {
            stable = raw;
            count = 0;
            if (stable == 0) {                      /* 눌림 확정 */
                pressed_at = now_ms;
                long_reported = 0;
            } else if (!long_reported) {            /* 뗌 확정, 길게 누름이 아니었다면 */
                return BUTTON_SHORT;
            }
        }
    } else {
        count = 0;
    }

    if (stable == 0 && !long_reported && now_ms - pressed_at >= LONG_PRESS_MS) {
        long_reported = 1;
        return BUTTON_LONG;
    }
    return BUTTON_NONE;
}
