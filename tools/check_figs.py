#!/usr/bin/env python3
"""원고의 그림 참조를 검사한다.

    python3 tools/check_figs.py

- 본문이 넣는 그림 파일이 실제로 있는지
- 장마다 그림 번호가 본문 등장 순서대로 매겨졌는지
- 본문에 넣은 그림이 docs/outline.md의 그림 목록에 있는지
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMG = re.compile(r"!\[그림 (\d+)-(\d+)[^\]]*\]\(\.\./(figures/[^)]+)\)")


def fig_key(minor):
    m = re.match(r"(\d+)([a-z]?)", minor)
    return int(m.group(1)), m.group(2)


def main():
    outline = (ROOT / "docs" / "outline.md").read_text(encoding="utf-8")
    listed = set(re.findall(r"^\s+- (\d+-\d+[a-z]?) ", outline, re.M))
    problems = 0
    for ch in sorted((ROOT / "chapters").glob("ch*.md")):
        text = ch.read_text(encoding="utf-8")
        refs = IMG.findall(text)
        prev = None
        for major, minor, path in refs:
            label = f"{major}-{minor}"
            if not (ROOT / path).is_file():
                print(f"{ch.name}: 그림 {label} 파일 없음 ({path})")
                problems += 1
            if label not in listed:
                print(f"{ch.name}: 그림 {label}이 outline.md 목록에 없음")
                problems += 1
            key = int(minor)
            if prev is not None and key <= prev:
                print(f"{ch.name}: 그림 {label}이 앞 그림보다 번호가 작거나 같음 (등장 순서 확인)")
                problems += 1
            prev = key
    print("문제 없음" if problems == 0 else f"문제 {problems}건")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
