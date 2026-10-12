/*
 * 32장 실습: ESP32 OTA와 롤백 (ESP-IDF 5.x)
 *
 *   1. 부팅하면 지금 파티션이 "검증 대기(PENDING_VERIFY)"인지 본다. 새로 받은 이미지의 첫 부팅이다.
 *   2. 자가 진단에 통과하면 esp_ota_mark_app_valid_cancel_rollback()으로 확정,
 *      실패하면 esp_ota_mark_app_invalid_rollback_and_reboot()로 이전 슬롯으로 돌아간다.
 *      확정하기 전에 리셋되면(멈춤, 워치독, 전원) 2단계 부트로더가 다음 부팅에 롤백한다.
 *   3. 1분마다 서버의 이미지를 확인하고, 버전이 다르면 받아서 다른 OTA 슬롯에 쓰고 재부팅.
 * STM32 부트로더(stm32/)가 직접 한 일을 ESP-IDF의 부트로더와 app_update 컴포넌트가 해 준다.
 */
#include <string.h>

#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"

static const char *TAG = "ota";

static bool self_test(void)
{
#if CONFIG_EXAMPLE_SIMULATE_BAD_IMAGE
    ESP_LOGW(TAG, "self-test: simulated failure");
    return false;
#else
    /* 실제 제품이라면: 센서 응답, 설정 읽기, 서버 접속(시간 제한 안에) 등 핵심 기능을 확인한다 */
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT) > 50 * 1024;
#endif
}

static void check_and_update(const esp_app_desc_t *running)
{
    esp_http_client_config_t http = {
        .url = CONFIG_EXAMPLE_OTA_URL,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t cfg = { .http_config = &http };
    esp_https_ota_handle_t h = NULL;

    if (esp_https_ota_begin(&cfg, &h) != ESP_OK) {
        ESP_LOGW(TAG, "server not reachable: %s", CONFIG_EXAMPLE_OTA_URL);
        return;
    }
    esp_app_desc_t incoming;
    if (esp_https_ota_get_img_desc(h, &incoming) != ESP_OK) {   /* 이미지 앞부분의 앱 설명만 먼저 읽는다 */
        esp_https_ota_abort(h);
        return;
    }
    if (strcmp(incoming.version, running->version) == 0) {
        ESP_LOGI(TAG, "already running %s", running->version);
        esp_https_ota_abort(h);
        return;
    }
    ESP_LOGI(TAG, "downloading %s -> %s", running->version, incoming.version);

    esp_err_t err;
    while ((err = esp_https_ota_perform(h)) == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
        /* 조금씩 받아 다른 OTA 슬롯에 쓴다 */
    }
    if (err == ESP_OK && esp_https_ota_is_complete_data_received(h)) {
        if (esp_https_ota_finish(h) == ESP_OK) {       /* 검증 후 다음 부팅 슬롯을 바꾼다 */
            ESP_LOGI(TAG, "update done, restarting");
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
        ESP_LOGE(TAG, "image validation failed");
    } else {
        ESP_LOGE(TAG, "download failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(h);
    }
}

void app_main(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_app_desc_t *desc = esp_app_get_description();
    ESP_LOGI(TAG, "version %s running from %s at 0x%lx", desc->version, running->label,
             (unsigned long)running->address);

    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "first boot of a new image: running self-test");
        if (self_test()) {
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "image confirmed");
        } else {
            ESP_LOGE(TAG, "self-test failed: rolling back");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }

    wifi_start_and_wait();
    for (;;) {
        check_and_update(desc);
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
