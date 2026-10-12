// 19장 실습 1: BLE 주변장치 (ESP32 + 아두이노 IDE)
//
// 서비스 두 개를 광고한다.
//   1) 환경 감지 서비스(표준 UUID 0x181A)의 온도 특성(0x2A6E): int16, 0.01 °C 단위, 읽기 + 알림
//      여기서는 GPIO34의 가변저항 값을 0~40 °C로 바꿔 "가짜 온도"로 쓴다.
//   2) LED 제어 서비스(직접 정한 128비트 UUID): 1바이트 쓰기, 0 = 끔, 1 = 켬 (GPIO4)
//
// 스마트폰의 nRF Connect 같은 BLE 앱으로 "ch19-ble"에 연결해 확인한다.
// 아두이노 ESP32 코어 3.x 기준. [확인 필요: 코어 버전별 BLE 라이브러리 API]

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

const int LED_PIN = 4;
const int POT_PIN = 34;      // ADC1 채널: Wi-Fi를 켜도 쓸 수 있다 (ADC2는 Wi-Fi와 충돌)

#define LED_SERVICE_UUID "6f1d0001-5c2b-4a8e-9a3d-7e4f2b6c9a01"   // 직접 만든 UUID (예)
#define LED_CHAR_UUID    "6f1d0002-5c2b-4a8e-9a3d-7e4f2b6c9a01"

BLECharacteristic *tempChar;
bool connected = false;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *s) override {
        connected = true;
    }
    void onDisconnect(BLEServer *s) override {
        connected = false;
        BLEDevice::startAdvertising();          // 연결이 끊기면 다시 광고
    }
};

class LedCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) override {
        String v = c->getValue();
        if (v.length() > 0) {
            digitalWrite(LED_PIN, v[0] ? HIGH : LOW);
        }
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    BLEDevice::init("ch19-ble");
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    BLEService *ess = server->createService(BLEUUID((uint16_t)0x181A));
    tempChar = ess->createCharacteristic(BLEUUID((uint16_t)0x2A6E),
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    tempChar->addDescriptor(new BLE2902());     // CCCD: 클라이언트가 알림을 켜고 끈다
    ess->start();

    BLEService *led = server->createService(LED_SERVICE_UUID);
    BLECharacteristic *ledChar = led->createCharacteristic(LED_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    ledChar->setCallbacks(new LedCallbacks());
    led->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLEUUID((uint16_t)0x181A));
    adv->start();
    Serial.println("advertising as ch19-ble");
}

void loop() {
    int raw = analogRead(POT_PIN);                       // 0~4095
    int16_t c100 = (int16_t)(raw * 4000L / 4095);        // 0~40.00 °C (가짜 온도)
    uint8_t le[2] = { (uint8_t)(c100 & 0xFF), (uint8_t)(c100 >> 8) };   // BLE는 리틀 엔디언
    tempChar->setValue(le, 2);
    if (connected) {
        tempChar->notify();
    }
    delay(1000);
}
