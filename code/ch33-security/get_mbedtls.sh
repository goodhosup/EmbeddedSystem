#!/bin/sh
# Mbed TLS(Apache 2.0)를 lib/mbedtls 아래로 받는다. 3.6은 장기 지원(LTS) 판이다.
# 이 책은 v3.6.7을 기준으로 했다. [확인 필요: 출간 시점 Mbed TLS 버전]
set -e
TAG=v3.6.7
DIR=lib/mbedtls

if [ ! -f "$DIR/library/ecdsa.c" ]; then
    git clone --depth 1 --branch "$TAG" https://github.com/Mbed-TLS/mbedtls.git "$DIR"
fi
echo "done: $DIR"
