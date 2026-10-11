/*
 * 4장: 비트 조작 매크로
 *
 * 인자에 괄호를 씌우고, 시프트할 1에는 접미사 u를 붙여 부호 있는 int의
 * 오버플로를 피한다. 32비트를 넘는 위치가 필요하면 1ull을 쓴다.
 */
#ifndef BITS_H
#define BITS_H

#define BIT(n)                (1u << (n))

#define SET_BIT(reg, n)       ((reg) |=  BIT(n))
#define CLEAR_BIT(reg, n)     ((reg) &= ~BIT(n))
#define TOGGLE_BIT(reg, n)    ((reg) ^=  BIT(n))
#define READ_BIT(reg, n)      (((reg) >> (n)) & 1u)

/* 여러 비트로 된 필드: pos부터 width비트 */
#define FIELD_MASK(pos, width)        (((1u << (width)) - 1u) << (pos))
#define FIELD_GET(reg, pos, width)    (((reg) & FIELD_MASK(pos, width)) >> (pos))
#define FIELD_SET(reg, pos, width, v) \
    ((reg) = ((reg) & ~FIELD_MASK(pos, width)) | (((v) << (pos)) & FIELD_MASK(pos, width)))

#endif /* BITS_H */
