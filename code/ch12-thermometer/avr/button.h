/* 버튼: 디바운싱과 짧게/길게 누름 구분 */
#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_SHORT,       /* 1초 안에 뗌 */
    BUTTON_LONG         /* 1초 이상 누르고 있음 (누르는 동안 한 번만) */
} button_event_t;

void           button_init(void);
button_event_t button_poll(uint32_t now_ms);   /* 5 ms마다 부른다 */

#endif
