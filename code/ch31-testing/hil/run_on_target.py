#!/usr/bin/env python3
"""31장: 보드에서 실행한 Unity 테스트 결과를 시리얼로 읽어 판정한다 (HIL의 가장 단순한 형태).

    python3 run_on_target.py --port /dev/ttyACM0 --flash

--flash를 주면 ../stm32에서 make flash로 테스트 펌웨어를 올린 뒤 결과를 기다린다.
요약 줄 "N Tests M Failures K Ignored"와 그 다음의 OK/FAIL을 찾고,
실패가 있거나 시간 안에 요약이 오지 않으면 0이 아닌 종료 코드를 돌려준다(36장의 CI에서 쓴다).
pyserial이 필요하다: pip install pyserial
"""
import argparse
import re
import subprocess
import sys
import time
from pathlib import Path

import serial

SUMMARY = re.compile(r"(\d+) Tests (\d+) Failures (\d+) Ignored")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=30.0, help="결과를 기다리는 최대 시간(초)")
    ap.add_argument("--flash", action="store_true", help="먼저 make flash로 펌웨어를 올린다")
    args = ap.parse_args()

    # 포트를 먼저 열어 두어야 리셋 직후의 출력을 놓치지 않는다
    with serial.Serial(args.port, args.baud, timeout=0.5) as port:
        port.reset_input_buffer()
        if args.flash:
            stm32_dir = Path(__file__).resolve().parent.parent / "stm32"
            r = subprocess.run(["make", "flash"], cwd=stm32_dir)
            if r.returncode != 0:
                print("flash failed", file=sys.stderr)
                return 2

        deadline = time.monotonic() + args.timeout
        summary = None
        while time.monotonic() < deadline:
            line = port.readline().decode("utf-8", errors="replace").rstrip()
            if not line:
                continue
            print(line)
            m = SUMMARY.search(line)
            if m:
                summary = tuple(int(x) for x in m.groups())
            elif summary and line in ("OK", "FAIL"):
                tests, failures, ignored = summary
                print(f"[hil] {tests} tests, {failures} failures, {ignored} ignored")
                return 0 if line == "OK" and failures == 0 else 1

    print("[hil] no test summary within timeout", file=sys.stderr)
    return 3


if __name__ == "__main__":
    sys.exit(main())
