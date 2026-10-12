# 20장 실습: ESP-IDF MQTT 장치
#   idf.py set-target esp32
#   idf.py menuconfig      → "Example configuration": Wi-Fi, 브로커 주소
#   idf.py build flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ch20-mqtt)
