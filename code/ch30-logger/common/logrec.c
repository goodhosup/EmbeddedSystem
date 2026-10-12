#include "logrec.h"

/* snprintf 없이 직접 숫자를 쓴다: newlib의 printf 계열은 Flash와 스택을 많이 쓴다 (25장) */
static size_t put_u32(char *p, uint32_t v)
{
    char tmp[10];
    size_t n = 0;
    do {
        tmp[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    for (size_t i = 0; i < n; i++) {
        p[i] = tmp[n - 1u - i];
    }
    return n;
}

size_t logrec_format(char *buf, size_t n, const logrec_t *r)
{
    char line[LOGREC_MAX_LINE];
    size_t k = 0;

    k += put_u32(&line[k], r->seq);
    line[k++] = ',';
    k += put_u32(&line[k], r->uptime_s);
    line[k++] = ',';
    if (r->flags & LOGREC_F_SENSOR_ERR) {
        /* 값 칸을 비워 둔다: 스프레드시트에서 0도로 오해하지 않게 */
    } else {
        int32_t t = r->temp_c100;
        uint32_t a = (t < 0) ? (uint32_t)(-(int64_t)t) : (uint32_t)t;
        if (t < 0) {
            line[k++] = '-';
        }
        k += put_u32(&line[k], a / 100u);
        line[k++] = '.';
        line[k++] = (char)('0' + (a / 10u) % 10u);
        line[k++] = (char)('0' + a % 10u);
    }
    line[k++] = ',';
    k += put_u32(&line[k], r->flags);
    line[k++] = '\r';
    line[k++] = '\n';

    if (k + 1u > n) {
        return 0;
    }
    for (size_t i = 0; i < k; i++) {
        buf[i] = line[i];
    }
    buf[k] = '\0';
    return k;
}
