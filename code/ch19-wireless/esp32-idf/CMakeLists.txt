# 19장 실습 3: ESP-IDF로 Wi-Fi 스테이션 + HTTP GET
#   idf.py set-target esp32
#   idf.py menuconfig      → "Example configuration"에서 SSID, 암호 입력
#   idf.py build flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ch19-wifi)
