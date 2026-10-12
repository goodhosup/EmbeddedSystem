# 링크 프로토콜(../../common/link.c)은 STM32 노드와 같은 파일, Wi-Fi는 20장의 wifi.c
idf_component_register(SRCS "main.c" "../../common/link.c" "../../../ch20-iot/esp32-idf/main/wifi.c"
                       INCLUDE_DIRS "." "../../common" "../../../ch20-iot/esp32-idf/main"
                       EMBED_TXTFILES "ca_cert.pem"
                       PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash mqtt esp_http_client driver json esp_timer)
