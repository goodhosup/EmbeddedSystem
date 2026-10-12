# 19장 실습: BLE와 Wi-Fi (ESP32)

| 폴더 | 도구 | 내용 |
| --- | --- | --- |
| `esp32-arduino/ble_sensor` | 아두이노 IDE (ESP32 코어 3.x) | BLE 주변장치: 표준 온도 특성(0x2A6E) 알림, LED 쓰기 특성 |
| `esp32-arduino/wifi_scan` | 아두이노 IDE | 주변 AP 스캔(RSSI), 접속, IP 출력 |
| `esp32-idf` | ESP-IDF 5.x | Wi-Fi 스테이션, 이벤트 처리, 재접속, HTTP GET |

연결: GPIO4에 LED + 330 Ω, GPIO34에 가변저항(3.3 V–GND) 가운데 다리.

BLE는 스마트폰의 범용 BLE 앱(예: nRF Connect)으로 "ch19-ble"를 찾아 연결해 확인합니다. Wi-Fi는 2.4 GHz 대역 AP에만 접속할 수 있습니다(ESP32는 5 GHz를 지원하지 않음).
