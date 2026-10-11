# 5장 실습: 네 보드에서 Blink

| 폴더 | 보드 | 도구 |
| --- | --- | --- |
| `avr/` | 아두이노 우노, ATmega128 | avr-gcc, avrdude (우노: 부트로더, ATmega128: USBasp) |
| `stm32/` | NUCLEO-F411RE | arm-none-eabi-gcc, OpenOCD, GDB, VS Code(Cortex-Debug) |
| `esp32-arduino/` | ESP32-DevKitC | 아두이노 IDE (ESP32 코어) |
| `esp32-idf/` | ESP32-DevKitC | ESP-IDF (`idf.py`) |

`stm32/.vscode/launch.json`은 VS Code의 Cortex-Debug 확장을 위한 설정입니다. 주변장치 레지스터 창을 보려면 ST 웹사이트에서 STM32F411의 SVD 파일을 받아 `stm32/STM32F411.svd`로 저장합니다(이 저장소에는 포함하지 않습니다).
