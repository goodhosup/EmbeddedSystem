/*
 * 20장 실습: MQTT로 텔레메트리 보내고 명령 받기 (ESP-IDF 5.x, esp-mqtt)
 *
 * 토픽 (장치 ID = Wi-Fi MAC 주소 뒤 3바이트)
 *   embbook/<id>/telemetry  ← 10초마다 {"uptime":...,"rssi":...,"heap":...}  (QoS 0)
 *   embbook/<id>/status     ← 접속하면 "online" (보존 메시지),
 *                             끊기면 브로커가 대신 "offline" (Last Will)
 *   embbook/<id>/cmd        → "led on" / "led off" (QoS 1로 구독)
 *
 * PC에서 확인 (Mosquitto 클라이언트):
 *   mosquitto_sub -h <브로커> -t 'embbook/#' -v
 *   mosquitto_pub -h <브로커> -t 'embbook/<id>/cmd' -m 'led on'
 */
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "wifi.h"

#define LED_GPIO GPIO_NUM_4

static const char *TAG = "ch20";
static char s_id[8];
static char s_topic_tel[48], s_topic_status[48], s_topic_cmd[48];
static esp_mqtt_client_handle_t s_client;
static volatile bool s_connected;

static void on_mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_mqtt_event_handle_t e = (esp_mqtt_event_handle_t)data;

    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to broker");
        s_connected = true;
        esp_mqtt_client_publish(e->client, s_topic_status, "online", 0, 1, 1);  /* QoS 1, 보존 */
        esp_mqtt_client_subscribe(e->client, s_topic_cmd, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected (the client reconnects automatically)");
        s_connected = false;
        break;
    case MQTT_EVENT_DATA:
        /* topic과 data는 널 문자로 끝나지 않으므로 길이를 함께 쓴다 */
        if (e->topic_len == (int)strlen(s_topic_cmd) && memcmp(e->topic, s_topic_cmd, e->topic_len) == 0) {
            if (e->data_len == 6 && memcmp(e->data, "led on", 6) == 0) {
                gpio_set_level(LED_GPIO, 1);
            } else if (e->data_len == 7 && memcmp(e->data, "led off", 7) == 0) {
                gpio_set_level(LED_GPIO, 0);
            } else {
                ESP_LOGW(TAG, "unknown command: %.*s", e->data_len, e->data);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

static void make_topics(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_id, sizeof s_id, "%02x%02x%02x", mac[3], mac[4], mac[5]);
    snprintf(s_topic_tel, sizeof s_topic_tel, "embbook/%s/telemetry", s_id);
    snprintf(s_topic_status, sizeof s_topic_status, "embbook/%s/status", s_id);
    snprintf(s_topic_cmd, sizeof s_topic_cmd, "embbook/%s/cmd", s_id);
    ESP_LOGI(TAG, "device id %s", s_id);
}

void app_main(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    wifi_start_and_wait();
    make_topics();

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_EXAMPLE_BROKER_URI,
        .session.keepalive = 30,                       /* 30초마다 생존 확인 */
        .session.last_will = {
            .topic = s_topic_status,
            .msg = "offline",
            .msg_len = 7,
            .qos = 1,
            .retain = 1,
        },
    };
    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, on_mqtt_event, NULL);
    esp_mqtt_client_start(s_client);

    for (;;) {
        if (s_connected) {
            char json[96];
            int n = snprintf(json, sizeof json, "{\"uptime\":%lld,\"rssi\":%d,\"heap\":%lu}",
                             (long long)(esp_timer_get_time() / 1000000), wifi_rssi(),
                             (unsigned long)esp_get_free_heap_size());
            esp_mqtt_client_publish(s_client, s_topic_tel, json, n, 0, 0);    /* QoS 0 */
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
