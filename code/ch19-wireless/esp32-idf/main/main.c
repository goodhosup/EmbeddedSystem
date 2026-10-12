/*
 * 19장 실습 3: ESP-IDF Wi-Fi 스테이션 + HTTP GET (ESP-IDF 5.x)
 *
 * 흐름 (19.6절)
 *   NVS 초기화 → 네트워크 인터페이스와 이벤트 루프 → Wi-Fi 드라이버 시작
 *   → STA_START 이벤트에서 접속 → GOT_IP 이벤트에서 IP 획득 → HTTP GET
 * 끊기면 다시 접속한다.
 */
#include <string.h>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "ch19";
static EventGroupHandle_t s_events;
#define GOT_IP_BIT BIT0

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, retrying");
        xEventGroupClearBits(s_events, GOT_IP_BIT);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_events, GOT_IP_BIT);
    }
}

static void wifi_start(void)
{
    ESP_ERROR_CHECK(esp_netif_init());                   /* LwIP와 네트워크 인터페이스 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());    /* 이벤트 루프 */
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL, NULL));

    wifi_config_t wc = { 0 };
    strlcpy((char *)wc.sta.ssid, CONFIG_EXAMPLE_WIFI_SSID, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, CONFIG_EXAMPLE_WIFI_PASSWORD, sizeof(wc.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());                   /* → WIFI_EVENT_STA_START */
}

static void http_get(void)
{
    esp_http_client_config_t hc = { .url = CONFIG_EXAMPLE_HTTP_URL };
    esp_http_client_handle_t c = esp_http_client_init(&hc);
    esp_err_t err = esp_http_client_perform(c);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP %d, content length %lld", esp_http_client_get_status_code(c),
                 (long long)esp_http_client_get_content_length(c));
    } else {
        ESP_LOGE(TAG, "HTTP failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(c);
}

void app_main(void)
{
    esp_err_t r = nvs_flash_init();                      /* Wi-Fi 드라이버가 보정값을 NVS에 저장한다 */
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    s_events = xEventGroupCreate();
    wifi_start();

    for (;;) {
        xEventGroupWaitBits(s_events, GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            ESP_LOGI(TAG, "RSSI %d dBm", ap.rssi);
        }
        http_get();
        vTaskDelay(pdMS_TO_TICKS(10000));                /* 10초마다 반복 */
    }
}
