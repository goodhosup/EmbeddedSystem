#!/bin/sh
# TinyUSB(MIT 라이선스)와 꼭 필요한 의존 파일 두 가지만 lib/ 아래로 받는다.
#   - ST의 STM32F4 CMSIS 디바이스 헤더 (cmsis_device_f4)
#   - Arm의 CMSIS 코어 헤더 (CMSIS_5의 CMSIS/Core/Include만, 부분 받기)
# TinyUSB의 tools/get_deps.py는 다른 라이브러리까지 수백 MB를 받으므로 쓰지 않는다.
# 이 책은 아래 TinyUSB 태그를 기준으로 했다. [확인 필요: 출간 시점 TinyUSB 버전]
set -e
TAG=0.17.0
TUSB=lib/tinyusb

if [ ! -d "$TUSB" ]; then
    git clone --depth 1 --branch "$TAG" https://github.com/hathach/tinyusb.git "$TUSB"
fi
if [ ! -d "$TUSB/hw/mcu/st/cmsis_device_f4" ]; then
    git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f4.git "$TUSB/hw/mcu/st/cmsis_device_f4"
fi
if [ ! -d "$TUSB/lib/CMSIS_5/CMSIS/Core/Include" ]; then
    rm -rf "$TUSB/lib/CMSIS_5"
    git clone --depth 1 --filter=blob:none --sparse https://github.com/ARM-software/CMSIS_5.git "$TUSB/lib/CMSIS_5"
    git -C "$TUSB/lib/CMSIS_5" sparse-checkout set CMSIS/Core/Include
fi
echo "done: $TUSB"
