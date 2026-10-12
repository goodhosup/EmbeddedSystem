/*
 * 40장: ESP32 Wi-Fi 연결부
 *
 *   UART1 (GPIO17 = TX → STM32 PA10, GPIO16 = RX ← STM32 PA9)로 센서 노드와 40장 링크 프로토콜로 대화한다.
 *   - 노드의 MSG_REPORT → MQTT(TLS) "nodes/<id>/telemetry"에 JSON으로 발행 (20장)
 *   - MQTT "nodes/<id>/ota"에 {"url": "...", "version": "1.1.0"}이 오면
 *     HTTPS로 서명된 이미지(.simg)를 받아 조각조각 노드에 보낸다. 서명 확인은 노드가 한다 (33장):
 *     연결부가 해킹당해도 가짜 펌웨어는 노드에서 거부된다.
 * UART는 link_task 하나가 평소에 읽고, OTA 중에는 ota_task가 뮤텍스로 넘겨받는다 (27장의 소유권).
 */
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/uart.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "link.h"
#include "mqtt_client.h"
#include "wifi.h"

#define LINK_UART  UART_NUM_1
#define TX_PIN     17
#define RX_PIN     16
#define CHUNK      128

static const char *TAG = "bridge";
extern const char ca_pem[] asm("_binary_ca_cert_pem_start");

static esp_mqtt_client_handle_t mqtt;
static SemaphoreHandle_t uart_lock;
static QueueHandle_t ota_q;
static uint8_t tx_seq;
static char topic_tel[64], topic_ota[64];

typedef struct { char url[192]; char version[16]; } ota_req_t;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void link_write(const link_msg_t *m)
{
    uint8_t f[LINK_FRAME_MAX];
    size_t n = link_encode(m, f, sizeof f);
    uart_write_bytes(LINK_UART, f, n);
}

static void publish_report(const link_msg_t *m)
{
    if (m->len < 13) return;
    uint32_t seq, up; int32_t t;
    memcpy(&seq, m->data, 4); memcpy(&up, &m->data[4], 4); memcpy(&t, &m->data[8], 4);
    char json[128];
    int n = snprintf(json, sizeof json, "{\"seq\":%lu,\"uptime\":%lu,\"temp_c\":%ld.%02ld,\"sensor_ok\":%s}",
                     (unsigned long)seq, (unsigned long)up, (long)(t / 100), (long)((t < 0 ? -t : t) % 100),
                     m->data[12] ? "false" : "true");
    esp_mqtt_client_publish(mqtt, topic_tel, json, n, 1, 0);       /* QoS 1 (20장) */
}

/* ---- 평소: 노드가 보내는 것을 읽어 발행 ---- */
static void link_task(void *arg)
{
    link_rx_t rx;
    link_rx_init(&rx);
    uint8_t b[64];
    link_msg_t m;
    uint32_t last_ping = 0;
    for (;;) {
        xSemaphoreTake(uart_lock, portMAX_DELAY);
        if (now_ms() - last_ping > 5000u) {             /* 노드의 자가 진단이 "연결부 응답"을 확인할 수 있게 */
            link_msg_t ping = { MSG_PING, tx_seq++, 0, {0} };
            link_write(&ping);                          /* 노드의 ACK는 확인하지 않는다 */
            last_ping = now_ms();
        }
        int n = uart_read_bytes(LINK_UART, b, sizeof b, pdMS_TO_TICKS(50));
        xSemaphoreGive(uart_lock);
        for (int i = 0; i < n; i++) {
            if (link_rx_byte(&rx, b[i], &m) && m.type == MSG_REPORT) publish_report(&m);
        }
        if (n <= 0) vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---- 신뢰 전송: ACK가 올 때까지 다시 보낸다 (정지-대기 ARQ) ---- */
static int send_reliable(arq_t *arq, link_rx_t *rx, uint8_t type, const void *data, uint16_t len, uint32_t timeout_ms)
{
    arq->timeout_ms = timeout_ms;      /* 노드가 오래 걸리는 일(섹터 지우기, 서명 확인)을 하는 명령은 길게 */
    link_msg_t m = { type, arq_start(arq, now_ms()), len, {0} };
    memcpy(m.data, data, len);
    link_write(&m);
    uint8_t b[32];
    link_msg_t r;
    for (;;) {
        int n = uart_read_bytes(LINK_UART, b, sizeof b, pdMS_TO_TICKS(10));
        for (int i = 0; i < n; i++) {
            if (link_rx_byte(rx, b[i], &r) && (r.type == MSG_ACK || r.type == MSG_NACK) && arq_on_ack(arq, r.seq)) {
                return r.type == MSG_ACK ? 0 : -1;             /* NACK: 노드가 거부함 (r.data[0]에 이유) */
            }
        }
        arq_action_t a = arq_poll(arq, now_ms());
        if (a == ARQ_RESEND) link_write(&m);
        if (a == ARQ_FAILED) return -2;
    }
}

static int parse_version(const char *s)
{
    int a = 0, b = 0, c = 0;
    sscanf(s, "%d.%d.%d", &a, &b, &c);
    return (a << 16) | (b << 8) | c;
}

static void ota_task(void *arg)
{
    ota_req_t req;
    static uint8_t buf[4 + CHUNK];
    for (;;) {
        xQueueReceive(ota_q, &req, portMAX_DELAY);
        ESP_LOGI(TAG, "OTA %s from %s", req.version, req.url);
        esp_http_client_config_t hc = { .url = req.url, .cert_pem = ca_pem, .timeout_ms = 10000 };
        esp_http_client_handle_t http = esp_http_client_init(&hc);
        if (esp_http_client_open(http, 0) != ESP_OK) { esp_http_client_cleanup(http); continue; }
        int total = (int)esp_http_client_fetch_headers(http);

        xSemaphoreTake(uart_lock, portMAX_DELAY);          /* OTA 동안 UART는 이 태스크가 쓴다 */
        /* seq는 세션이 바뀌어도 이어진다: 새 세션의 첫 프레임이 지난 세션의 마지막 seq와 같으면
           노드가 중복으로 보고 처리하지 않기 때문이다 */
        static arq_t arq;
        static bool arq_ready;
        if (!arq_ready) { arq_init(&arq, 200, 5); arq_ready = true; }
        arq.retransmits = 0;
        link_rx_t rx;
        link_rx_init(&rx);
        uint32_t hdr[2] = { (uint32_t)total, (uint32_t)parse_version(req.version) };
        int r = (total > 0) ? send_reliable(&arq, &rx, MSG_OTA_BEGIN, hdr, sizeof hdr, 4000) : -3;   /* 노드가 128 KB 섹터를 지운다: 1~2 s */
        uint32_t off = 0;
        while (r == 0 && off < (uint32_t)total) {
            int n = esp_http_client_read(http, (char *)&buf[4], CHUNK);
            if (n <= 0) { r = -4; break; }
            memcpy(buf, &off, 4);
            r = send_reliable(&arq, &rx, MSG_OTA_CHUNK, buf, (uint16_t)(4 + n), 200);
            off += (uint32_t)n;
        }
        if (r == 0) r = send_reliable(&arq, &rx, MSG_OTA_END, NULL, 0, 6000);   /* 노드가 서명을 확인한다: 1 s 이상 */
        xSemaphoreGive(uart_lock);
        esp_http_client_close(http);
        esp_http_client_cleanup(http);

        char msg[96];
        int n = snprintf(msg, sizeof msg, "{\"ota\":\"%s\",\"version\":\"%s\",\"retransmits\":%lu}",
                         r == 0 ? "accepted" : "failed", req.version, (unsigned long)arq.retransmits);
        esp_mqtt_client_publish(mqtt, topic_tel, msg, n, 1, 0);
        ESP_LOGI(TAG, "%s", msg);
    }
}

static void on_mqtt(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_mqtt_event_handle_t e = data;
    if (id == MQTT_EVENT_CONNECTED) {
        esp_mqtt_client_subscribe(mqtt, topic_ota, 1);
    } else if (id == MQTT_EVENT_DATA && e->topic_len == (int)strlen(topic_ota) &&
               strncmp(e->topic, topic_ota, e->topic_len) == 0) {
        cJSON *j = cJSON_ParseWithLength(e->data, e->data_len);    /* 바깥에서 온 데이터: 길이와 형식 확인 */
        const cJSON *url = cJSON_GetObjectItem(j, "url"), *ver = cJSON_GetObjectItem(j, "version");
        if (cJSON_IsString(url) && cJSON_IsString(ver) && strncmp(url->valuestring, "https://", 8) == 0) {
            ota_req_t req = { 0 };
            strlcpy(req.url, url->valuestring, sizeof req.url);
            strlcpy(req.version, ver->valuestring, sizeof req.version);
            xQueueSend(ota_q, &req, 0);                             /* 이벤트 처리기에서는 기다리지 않는다 */
        } else {
            ESP_LOGW(TAG, "bad OTA command ignored");
        }
        cJSON_Delete(j);
    }
}

void app_main(void)
{
    snprintf(topic_tel, sizeof topic_tel, "nodes/%s/telemetry", CONFIG_EXAMPLE_NODE_ID);
    snprintf(topic_ota, sizeof topic_ota, "nodes/%s/ota", CONFIG_EXAMPLE_NODE_ID);

    uart_config_t uc = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
                         .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT };
    ESP_ERROR_CHECK(uart_driver_install(LINK_UART, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(LINK_UART, &uc));
    ESP_ERROR_CHECK(uart_set_pin(LINK_UART, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    wifi_start_and_wait();

    esp_mqtt_client_config_t mc = {
        .broker.address.uri = CONFIG_EXAMPLE_BROKER_URI,
        .broker.verification.certificate = ca_pem,                  /* 서버 인증 (20장 TLS) */
        .session.last_will = { .topic = topic_tel, .msg = "{\"online\":false}", .qos = 1, .retain = 1 },
    };
    mqtt = esp_mqtt_client_init(&mc);
    esp_mqtt_client_register_event(mqtt, ESP_EVENT_ANY_ID, on_mqtt, NULL);
    esp_mqtt_client_start(mqtt);

    uart_lock = xSemaphoreCreateMutex();
    ota_q = xQueueCreate(1, sizeof(ota_req_t));
    xTaskCreate(link_task, "link", 4096, NULL, 5, NULL);
    xTaskCreate(ota_task, "ota", 8192, NULL, 4, NULL);
    ESP_LOGI(TAG, "bridge %s ready", CONFIG_EXAMPLE_NODE_ID);
}
