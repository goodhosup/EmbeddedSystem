# 32장 실습: ESP32 OTA (ESP-IDF 5.x)
#   idf.py set-target esp32
#   idf.py menuconfig      → "Example configuration": Wi-Fi, 업데이트 URL
#   idf.py build flash monitor
# 버전을 바꿔 다시 빌드한 build/ch32-ota.bin을 PC의 웹 서버에 올리면 장치가 받아 간다.
cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "1.0.0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ch32-ota)
