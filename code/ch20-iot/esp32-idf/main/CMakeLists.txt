idf_component_register(SRCS "main.c" "wifi.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash mqtt driver esp_timer)
