# 20장 실습: MQTT 장치 (ESP32 + ESP-IDF)

ESP32가 Wi-Fi로 MQTT 브로커에 접속해 10초마다 텔레메트리를 JSON으로 보내고, 명령 토픽으로 LED를 켜고 끕니다.

## 준비: PC에 브로커 띄우기

```bash
# 우분투/데비안
sudo apt install mosquitto mosquitto-clients
# Mosquitto 2.x는 기본으로 외부 접속을 막는다. 시험용 설정:
printf 'listener 1883\nallow_anonymous true\n' > test.conf
mosquitto -c test.conf -v
```

익명 접속을 허용하는 위 설정은 집 안 네트워크에서 시험할 때만 쓰세요(20.5절).

## 빌드

```bash
cd esp32-idf
idf.py set-target esp32
idf.py menuconfig        # Example configuration: SSID, 암호, 브로커 URI(mqtt://PC의 IP)
idf.py build flash monitor
```

## 확인

```bash
mosquitto_sub -h localhost -t 'embbook/#' -v
mosquitto_pub -h localhost -t 'embbook/<장치 ID>/cmd' -m 'led on'
```

GPIO4에 LED와 330 Ω 저항을 연결합니다.
