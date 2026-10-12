# Wi-Fi 연결은 20장 예제의 wifi.c를 그대로 쓴다
idf_component_register(SRCS "main.c" "../../../ch20-iot/esp32-idf/main/wifi.c"
                       INCLUDE_DIRS "." "../../../ch20-iot/esp32-idf/main"
                       PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash
                                     app_update esp_https_ota esp_http_client esp_app_format)
