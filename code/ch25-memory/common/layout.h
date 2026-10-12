/* 25장: 구조체 패딩과 정렬 예 (PC와 STM32에서 같은 결과) */
#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>

struct sensor_bad {          /* 선언 순서 그대로: 패딩이 생긴다 */
    uint8_t  id;
    uint32_t timestamp;
    uint8_t  flags;
    uint16_t value;
};

struct sensor_good {         /* 큰 멤버부터: 패딩이 줄어든다 */
    uint32_t timestamp;
    uint16_t value;
    uint8_t  id;
    uint8_t  flags;
};

struct __attribute__((packed)) sensor_packed {   /* 패딩 없음: 통신 프레임용. 멤버 접근이 느리거나 위험할 수 있다 */
    uint8_t  id;
    uint32_t timestamp;
    uint8_t  flags;
    uint16_t value;
};

#endif
