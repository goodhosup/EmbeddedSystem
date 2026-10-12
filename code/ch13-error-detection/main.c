/*
 * 13장 실습: 오류 검출 세 가지 (PC에서 실행)
 *   패리티, 8비트 체크섬, CRC-8
 *
 *   make run
 *
 * CRC-8 매개변수: 다항식 0x07 (x^8 + x^2 + x + 1), 초깃값 0x00, 반사 없음, 최종 XOR 없음.
 * 이 매개변수에서 "123456789"의 CRC는 0xF4다(널리 쓰이는 검증값).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t parity_even(uint8_t b)        /* 1의 개수가 짝수가 되도록 하는 패리티 비트 */
{
    uint8_t p = 0;
    while (b) {
        p ^= (uint8_t)(b & 1u);
        b >>= 1;
    }
    return p;
}

static uint8_t checksum8(const uint8_t *d, size_t n)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum = (uint8_t)(sum + d[i]);
    }
    return (uint8_t)(0x100 - sum);           /* 2의 보수: 데이터와 합하면 0 */
}

static uint8_t crc8(const uint8_t *d, size_t n)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < n; i++) {
        crc ^= d[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void show(const char *title, uint8_t *msg, size_t n)
{
    printf("%-26s 체크섬=0x%02X  CRC-8=0x%02X\n", title, checksum8(msg, n), crc8(msg, n));
}

int main(void)
{
    uint8_t msg[] = "123456789";
    size_t n = strlen((char *)msg);

    printf("'A'(0x41)의 짝수 패리티 비트 = %u\n", parity_even('A'));
    show("원래 메시지", msg, n);

    msg[3] ^= 0x01;                          /* 비트 하나 뒤집기 */
    show("비트 1개 오류", msg, n);
    msg[3] ^= 0x01;

    msg[2] ^= 0x01;                          /* 같은 자리 비트 두 개를 반대로 */
    msg[5] ^= 0x01;
    show("비트 2개 오류 (서로 상쇄)", msg, n);
    msg[2] ^= 0x01;
    msg[5] ^= 0x01;

    uint8_t t = msg[0];                      /* 두 바이트 순서 바꾸기 */
    msg[0] = msg[1];
    msg[1] = t;
    show("바이트 순서 뒤바뀜", msg, n);
    return 0;
}
