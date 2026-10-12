/* 31장: 보드에서 Unity를 쓸 때의 설정 (-DUNITY_INCLUDE_CONFIG_H 로 읽힌다) */
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

void uart2_putc(char c);
#define UNITY_OUTPUT_CHAR(a)   uart2_putc((char)(a))   /* 결과를 USART2로 */
#define UNITY_OUTPUT_FLUSH()
#define UNITY_EXCLUDE_FLOAT                            /* 이 시험은 실수를 비교하지 않는다: Flash 절약 */
#define UNITY_EXCLUDE_DOUBLE

#endif
