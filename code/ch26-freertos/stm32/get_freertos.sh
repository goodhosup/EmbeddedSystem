#!/bin/sh
# FreeRTOS 커널(MIT 라이선스)을 lib/FreeRTOS-Kernel 아래로 받는다.
# 이 책은 아래 태그를 기준으로 했다. [확인 필요: 출간 시점 FreeRTOS 버전]
set -e
TAG=V11.1.0
DIR=lib/FreeRTOS-Kernel

if [ ! -d "$DIR" ]; then
    git clone --depth 1 --branch "$TAG" https://github.com/FreeRTOS/FreeRTOS-Kernel.git "$DIR"
fi
echo "done: $DIR"
