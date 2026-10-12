#!/bin/sh
# Unity(MIT 라이선스, ThrowTheSwitch.org)를 lib/unity 아래로 받는다.
# 이 책은 v2.6.1을 기준으로 했다. [확인 필요: 출간 시점 Unity 버전]
set -e
TAG=v2.6.1
DIR=lib/unity

if [ ! -f "$DIR/src/unity.c" ]; then
    git clone --depth 1 --branch "$TAG" https://github.com/ThrowTheSwitch/Unity.git "$DIR"
fi
echo "done: $DIR/src"
