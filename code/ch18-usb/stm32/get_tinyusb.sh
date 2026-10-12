#!/bin/sh
# TinyUSB(MIT 라이선스)와 STM32F4용 의존 파일(CMSIS 디바이스 헤더)을 lib/ 아래로 받는다.
# 이 책은 아래 태그로 시험했다. [확인 필요: 출간 시점 TinyUSB 버전]
set -e
TAG=0.17.0
if [ ! -d lib/tinyusb ]; then
    git clone --depth 1 --branch "$TAG" https://github.com/hathach/tinyusb.git lib/tinyusb
fi
cd lib/tinyusb
python3 tools/get_deps.py stm32f4
