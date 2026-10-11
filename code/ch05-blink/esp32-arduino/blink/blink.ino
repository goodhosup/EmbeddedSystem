// 5장: 아두이노 IDE로 ESP32 Blink
// ESP32-DevKitC에는 사용자 LED가 없으므로 GPIO4에 LED와 330 Ω 저항을 직렬로 달아 GND에 연결한다.
const int LED_PIN = 4;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("ESP32 blink start");
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}
