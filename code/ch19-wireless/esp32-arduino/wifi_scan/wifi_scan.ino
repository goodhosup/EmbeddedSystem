// 19장 실습 2: Wi-Fi 스캔과 접속 (ESP32 + 아두이노 IDE)
// 주변 AP를 신호 세기(RSSI)와 함께 보여 준 뒤, 지정한 AP에 접속해 IP 주소를 출력한다.

#include <WiFi.h>

const char *WIFI_SSID = "your-ssid";        // 자신의 Wi-Fi 이름과 암호로 바꾼다
const char *WIFI_PASS = "your-password";

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    int n = WiFi.scanNetworks();
    Serial.printf("%d networks\n", n);
    for (int i = 0; i < n; i++) {
        Serial.printf("%2d  %-32s  %4d dBm  ch%2d\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("connecting");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("failed: check SSID/password, 2.4 GHz band");
    }
}

void loop() {
}
