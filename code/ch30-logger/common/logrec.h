/*
 * 30장: 측정 기록과 CSV 한 줄 만들기 (하드웨어와 무관, PC에서 시험)
 */
#ifndef LOGREC_H
#define LOGREC_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t seq;           /* 일련번호: 빠진 기록을 찾을 수 있게 */
    uint32_t uptime_s;      /* 켜진 뒤 초 (RTC가 없으므로) */
    int32_t  temp_c100;     /* 섭씨 × 100 (15장 bmp280_read_temp_c100) */
    uint16_t flags;         /* LOGREC_F_... */
} logrec_t;

#define LOGREC_F_SENSOR_ERR  (1u << 0)   /* 센서 읽기 실패: temp는 의미 없음 */
#define LOGREC_F_DROPPED     (1u << 1)   /* 이 기록 앞에서 큐가 넘쳐 버린 기록이 있음 */

#define LOGREC_HEADER "seq,uptime_s,temp_c,flags\r\n"
#define LOGREC_MAX_LINE 40

/* r을 CSV 한 줄로 buf에 쓴다. 쓴 글자 수(널 제외)를 돌려주고, 공간이 모자라면 0 */
size_t logrec_format(char *buf, size_t n, const logrec_t *r);

#endif
