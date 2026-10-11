/*
 * 5장: ESP-IDF로 ESP32 Blink
 * GPIO4에 LED와 330 Ω 저항을 직렬로 달아 GND에 연결한다.
 * ESP-IDF에서는 main() 대신 app_main()이 시작점이다. 그 전에 FreeRTOS가 이미 실행 중이다.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_4

void app_main(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    printf("ESP32 blink start\n");

    int level = 0;
    for (;;) {
        gpio_set_level(LED_GPIO, level);
        level = !level;
        vTaskDelay(pdMS_TO_TICKS(500));   /* 500 ms 동안 다른 태스크에 CPU를 양보한다 */
    }
}
