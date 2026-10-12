/*
 * 35장: RAM 시험 (March C-)
 *   ⇕(w0) ⇑(r0,w1) ⇑(r1,w0) ⇓(r0,w1) ⇓(r1,w0) ⇕(r0)
 *   고착 고장(stuck-at), 전이 고장, 많은 결합 고장(한 칸을 쓰면 다른 칸이 바뀜)을 잡는다.
 *   가전제품 안전 규격(IEC 60730의 클래스 B)이 요구하는 시험의 대표적인 예.
 * 메모리 접근은 함수로 받는다: PC 테스트에서 고장 난 메모리를 흉내 내려고.
 */
#ifndef MARCH_H
#define MARCH_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t (*rd)(void *ctx, size_t i);
    void     (*wr)(void *ctx, size_t i, uint32_t v);
    void      *ctx;
} mem_if_t;

/* n개 워드를 시험한다. 처음 발견한 고장 워드의 번호, 문제없으면 -1. 내용은 지워진다 */
long march_c_minus(const mem_if_t *m, size_t n);

#endif
