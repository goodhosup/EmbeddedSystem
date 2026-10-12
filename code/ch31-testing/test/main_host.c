/* 31장: PC에서 모든 테스트를 실행한다 */
#include "unity.h"

void run_thermostat_tests(void);
void run_bmp280_tests(void);

int main(void)
{
    UNITY_BEGIN();
    run_thermostat_tests();
    run_bmp280_tests();
    return UNITY_END();
}
