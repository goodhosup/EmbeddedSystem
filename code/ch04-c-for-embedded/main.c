/*
 * 4장 실습: 임베디드 C의 함정을 PC에서 확인하기
 *
 * 하드웨어 없이 PC의 gcc로 빌드해서 실행한다.
 *   make run
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bits.h"

/* ---- 1. 레지스터 맵 구조체와 오프셋 검사 ---- */
typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;    /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFR[2];    /* 0x20, 0x24 */
} GPIO_TypeDef;

/* 오프셋이 데이터시트와 다르면 컴파일이 실패한다 (C11) */
_Static_assert(offsetof(GPIO_TypeDef, ODR) == 0x14, "ODR offset");
_Static_assert(offsetof(GPIO_TypeDef, AFR) == 0x20, "AFR offset");
_Static_assert(sizeof(GPIO_TypeDef) == 0x28, "GPIO size");

static void demo_struct(void)
{
    /* 실제 주소 대신 PC 메모리의 가짜 레지스터에 구조체를 겹쳐 본다 */
    static uint32_t fake_gpio[10];
    GPIO_TypeDef *gpio = (GPIO_TypeDef *)fake_gpio;

    gpio->ODR |= BIT(5);
    printf("[구조체] fake_gpio[5] = 0x%08X (ODR은 0x14 / 4 = 5번째 워드)\n", (unsigned)fake_gpio[5]);
}

/* ---- 2. 비트 연산 ---- */
static void demo_bits(void)
{
    uint8_t x = 0xA6;                   /* 1010 0110 */

    SET_BIT(x, 3);    printf("[비트] set 3    : 0x%02X\n", x);   /* 0xAE */
    CLEAR_BIT(x, 2);  printf("[비트] clear 2  : 0x%02X\n", x);   /* 0xAA */
    TOGGLE_BIT(x, 7); printf("[비트] toggle 7 : 0x%02X\n", x);   /* 0x2A */
    printf("[비트] read 5   : %u\n", READ_BIT(x, 5));            /* 1 */

    uint32_t moder = 0xFFFFFFFFu;
    FIELD_SET(moder, 5 * 2, 2, 1u);     /* PA5 모드 = 01 (출력) */
    printf("[비트] MODER    : 0x%08X\n", (unsigned)moder);       /* 0xFFFFF7FF */
}

/* ---- 3. 정수 승격과 부호의 함정 ---- */
static void demo_promotion(void)
{
    uint8_t a = 200, b = 100;
    uint8_t sum8 = a + b;               /* 300이 8비트에 잘려 44 */
    int sum = a + b;                    /* int로 승격되어 300 */
    printf("[승격] uint8_t 합 = %u, int 합 = %d\n", sum8, sum);

    uint8_t mask = 0x0F;
    if (~mask == 0xF0) {
        printf("[승격] ~mask == 0xF0 : 참\n");
    } else {
        printf("[승격] ~mask == 0xF0 : 거짓! (~mask = 0x%X, int로 승격된 뒤 반전)\n", ~mask);
    }
    if ((uint8_t)~mask == 0xF0) {
        printf("[승격] (uint8_t)~mask == 0xF0 : 참\n");
    }

    int minus_one = -1;
    unsigned int one = 1;
    if (minus_one < one) {
        printf("[부호] -1 < 1u : 참\n");
    } else {
        printf("[부호] -1 < 1u : 거짓! (-1이 unsigned로 바뀌어 0x%X)\n", (unsigned)minus_one);
    }

    /* 16비트 int(AVR)를 흉내 낸다: int16_t로 계산하면 1 << 15는 음수가 된다 */
    int16_t avr_int = (int16_t)(1u << 15);
    printf("[크기] 16비트 int에서 1 << 15 = %d (AVR에서는 이렇게 된다)\n", avr_int);
    printf("[크기] 이 PC에서 sizeof(int) = %zu, AVR은 2, Cortex-M은 4\n", sizeof(int));
}

int main(void)
{
    demo_struct();
    demo_bits();
    demo_promotion();
    return 0;
}
