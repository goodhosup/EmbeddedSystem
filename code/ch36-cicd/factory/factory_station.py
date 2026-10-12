#!/usr/bin/env python3
"""36장: 생산 라인의 시험 PC 프로그램 (공장 시험 펌웨어와 UART로 대화)

    python3 factory_station.py --port /dev/ttyACM0 --log results.csv
    python3 factory_station.py --port /dev/ttyACM0 --log results.csv --provision --prefix EM26

시험 항목을 차례로 실행하고, 모두 통과하면(그리고 --provision이면) 일련번호를 정해 장치에 기록한 뒤
다시 읽어 확인한다. 결과는 장치의 고유 번호(UID)와 함께 CSV에 한 줄씩 남긴다: 나중에 현장에서 돌아온
장치가 언제, 어떤 시험을 통과했는지 추적할 수 있다. pyserial이 필요하다.
"""
import argparse
import csv
import datetime
import json
import sys
import time
from pathlib import Path

import serial

TESTS = [("RAM", "ram", "ok"), ("I2C", "bmp280", "ok"), ("GPIO", "gpio", "ok")]


def ask(port, cmd: str, timeout: float = 3.0) -> dict:
    port.reset_input_buffer()
    port.write((cmd + "\n").encode())
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        line = port.readline().decode(errors="replace").strip()
        if line.startswith("{"):
            return json.loads(line)
    raise TimeoutError(f"no answer to {cmd}")


def next_serial(prefix: str, counter_file: Path) -> str:
    n = int(counter_file.read_text()) + 1 if counter_file.exists() else 1
    counter_file.write_text(str(n))                # 실패해도 번호는 다시 쓰지 않는다 (중복 방지)
    return f"{prefix}{n:06d}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--log", default="results.csv")
    ap.add_argument("--provision", action="store_true", help="모두 통과하면 일련번호를 기록한다")
    ap.add_argument("--prefix", default="EM26")
    a = ap.parse_args()

    row = {"time": datetime.datetime.now().isoformat(timespec="seconds")}
    ok = True
    with serial.Serial(a.port, 115200, timeout=0.5) as port:
        time.sleep(0.5)
        ident = ask(port, "ID")
        row.update(uid=ident["uid"], flash_kb=ident["flash_kb"], fw=ident["fw"])
        for cmd, key, expect in TESTS:
            got = ask(port, cmd).get(key, "?")
            row[key] = got
            ok &= got == expect
            print(f"  {cmd:5s} {got}")
        if ok and a.provision:
            sn = next_serial(a.prefix, Path(a.log).with_suffix(".counter"))
            w = ask(port, f"SERIAL {sn}").get("serial")
            back = ask(port, "SERIAL?").get("serial")
            row["serial"] = sn if (w == "written" and back == sn) else "PROVISION-FAIL"
            ok &= row["serial"] == sn
    row["result"] = "PASS" if ok else "FAIL"

    new = not Path(a.log).exists()
    with open(a.log, "a", newline="") as f:
        wcsv = csv.DictWriter(f, fieldnames=["time", "uid", "flash_kb", "fw", "ram", "bmp280", "gpio", "serial", "result"])
        if new:
            wcsv.writeheader()
        wcsv.writerow(row)
    print(f"\n==== {row['result']} ==== uid {row['uid']} serial {row.get('serial', '-')}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
