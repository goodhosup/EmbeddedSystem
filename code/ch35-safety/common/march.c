#include "march.h"

#define P0 0x00000000u
#define P1 0xFFFFFFFFu

long march_c_minus(const mem_if_t *m, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) m->wr(m->ctx, i, P0);                      /* ⇕(w0) */
    for (i = 0; i < n; i++) {                                           /* ⇑(r0, w1) */
        if (m->rd(m->ctx, i) != P0) return (long)i;
        m->wr(m->ctx, i, P1);
    }
    for (i = 0; i < n; i++) {                                           /* ⇑(r1, w0) */
        if (m->rd(m->ctx, i) != P1) return (long)i;
        m->wr(m->ctx, i, P0);
    }
    for (i = n; i-- > 0; ) {                                            /* ⇓(r0, w1) */
        if (m->rd(m->ctx, i) != P0) return (long)i;
        m->wr(m->ctx, i, P1);
    }
    for (i = n; i-- > 0; ) {                                            /* ⇓(r1, w0) */
        if (m->rd(m->ctx, i) != P1) return (long)i;
        m->wr(m->ctx, i, P0);
    }
    for (i = 0; i < n; i++) {                                           /* ⇕(r0) */
        if (m->rd(m->ctx, i) != P0) return (long)i;
    }
    return -1;
}
