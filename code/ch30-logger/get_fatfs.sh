#!/bin/sh
# FatFs(ChaN, BSD 스타일 1조항 라이선스)를 lib/fatfs 아래로 받는다.
# 이 책은 R0.15a를 기준으로 했다. [확인 필요: 출간 시점 FatFs 버전과 내려받기 주소]
# 설정 파일은 이 프로젝트의 ffconf.h를 쓰므로, 배포본의 ffconf.h는 지운다
# (ff.c는 "ffconf.h"를 자기 폴더에서 먼저 찾기 때문).
set -e
VER=ff15a
DIR=lib/fatfs

if [ ! -f "$DIR/source/ff.c" ]; then
    mkdir -p "$DIR"
    curl -L -o "$DIR/$VER.zip" "http://elm-chan.org/fsw/ff/arc/$VER.zip"
    (cd "$DIR" && unzip -q -o "$VER.zip" && rm "$VER.zip")
fi
rm -f "$DIR/source/ffconf.h"
echo "done: $DIR/source"
