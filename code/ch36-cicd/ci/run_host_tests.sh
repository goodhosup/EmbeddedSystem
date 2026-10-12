#!/bin/sh
# 36장: 이 책의 모든 PC 테스트를 한 번에 실행한다 (CI의 첫 단계, 로컬에서도 그대로)
#
#   sh code/ch36-cicd/ci/run_host_tests.sh          저장소 맨 위에서
#
# 외부 라이브러리가 필요한 테스트는 먼저 받는다 (FatFs, Unity, Mbed TLS).
# 하나라도 실패하면 0이 아닌 값으로 끝나서 CI가 빌드를 실패로 표시한다.
set -u
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
cd "$ROOT/code"

(cd ch30-logger && ./get_fatfs.sh) >/dev/null
(cd ch31-testing && ./get_unity.sh) >/dev/null 2>&1
(cd ch33-security && ./get_mbedtls.sh) >/dev/null 2>&1

pass=0
fail=0
failed=""
for dir in */test ch28-realtime/host; do
    [ -f "$dir/Makefile" ] || continue
    if make -s -C "$dir" test >"/tmp/host-test-$$.log" 2>&1; then
        pass=$((pass + 1))
        printf "  ok    %s\n" "$dir"
    else
        fail=$((fail + 1))
        failed="$failed $dir"
        printf "  FAIL  %s\n" "$dir"
        sed 's/^/        /' "/tmp/host-test-$$.log" | tail -20
    fi
    make -s -C "$dir" clean >/dev/null 2>&1
done
rm -f "/tmp/host-test-$$.log"

echo "host tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ] || { echo "failed:$failed"; exit 1; }
