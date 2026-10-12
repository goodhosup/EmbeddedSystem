/*
 * 31장: 보드에서 단위 테스트 실행하기 (NUCLEO-F411RE)
 *
 * PC에서 돌린 test/의 테스트 파일을 그대로 크로스 컴파일해 보드에서 실행하고,
 * 결과를 USART2(115200 bps)로 출력한다. hil/run_on_target.py가 이 출력을 읽어 판정한다.
 * PC와 보드에서 결과가 다르다면 컴파일러, int 크기, 정렬, 엔디언 차이를 의심한다(4장, 25장).
 */
#include "unity.h"
#include "uart2.h"

void run_thermostat_tests(void);
void run_bmp280_tests(void);

int main(void)
{
    uart2_init(115200, 16000000u);
    for (volatile uint32_t i = 0; i < 200000u; i++) {   /* 터미널이 열릴 시간 (약 0.1 s) */
    }

    UNITY_BEGIN();
    run_thermostat_tests();
    run_bmp280_tests();
    (void)UNITY_END();

    for (;;) {
        __asm volatile ("wfi");
    }
}
