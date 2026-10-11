#!/usr/bin/env python3
"""원고와 예제 코드의 [확인 필요] 표시를 모아 docs/publish-checklist.md를 만든다.

    python3 tools/checklist.py

장을 고칠 때마다 다시 실행한다. 행 번호는 실행 시점 기준이다.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs" / "publish-checklist.md"
PATTERN = re.compile(r"\[확인 필요[^\]]*\]")
TARGETS = ["chapters", "appendix", "code", "README.md"]

HEADER = """# 출판 전 확인 목록

원고와 예제 코드에 남아 있는 `[확인 필요]` 표시를 모은 목록입니다. 편집 단계에서 한 번에 처리합니다.
이 파일은 `python3 tools/checklist.py`로 자동 생성되므로 직접 고치지 않습니다.

| 파일 | 행 | 내용 |
| --- | --- | --- |
"""


def files():
    for t in TARGETS:
        p = ROOT / t
        if p.is_file():
            yield p
        elif p.is_dir():
            for f in sorted(p.rglob("*")):
                if f.is_file() and f.suffix in {".md", ".c", ".h", ".ino", ".py", ".ld", ".mk", ".txt"} \
                        or f.name in {"Makefile", "CMakeLists.txt"}:
                    yield f


def main():
    rows = []
    for f in files():
        for n, line in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
            for m in PATTERN.finditer(line):
                rows.append((f.relative_to(ROOT).as_posix(), n, m.group(0)))
    body = "".join(f"| `{p}` | {n} | {t} |\n" for p, n, t in rows)
    if not rows:
        body = "| (없음) | | |\n"
    OUT.write_text(HEADER + body + f"\n총 {len(rows)}건\n", encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)} ({len(rows)}건)")


if __name__ == "__main__":
    main()
