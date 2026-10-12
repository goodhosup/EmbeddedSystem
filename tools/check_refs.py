#!/usr/bin/env python3
"""원고의 절 참조(예: "15.8절")를 검사한다.

    python3 tools/check_refs.py

- 참조한 절(N.M)이 해당 장 파일에 "## N.M " 제목으로 실제로 있는지 확인한다.
- (-v) "N.M절의 X"처럼 뒤에 말이 붙어 있으면, X의 첫 낱말이 그 절 제목에 들어 있는지
  확인하고 없으면 경고한다. (낱말이 달라도 맞는 참조일 수 있으므로 경고만 한다)
- 아직 쓰지 않은 장에 대한 참조는 건너뛴다.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REF = re.compile(r"(?<![\d.])(\d{1,2})\.(\d{1,2})절(?:의 ([^\s,.)]+))?")
VERBOSE = "-v" in sys.argv                  # 제목 낱말 비교 경고까지 보려면 -v
HEAD = re.compile(r"^## (\d+)\.(\d+) (.+)$", re.M)


def main():
    heads = {}
    for ch in (ROOT / "chapters").glob("ch*.md"):
        for a, b, title in HEAD.findall(ch.read_text(encoding="utf-8")):
            heads[(int(a), int(b))] = title
    written = {a for a, _ in heads}
    errors = warnings = 0
    for ch in sorted((ROOT / "chapters").glob("ch*.md")):
        for n, line in enumerate(ch.read_text(encoding="utf-8").splitlines(), 1):
            for m in REF.finditer(line):
                a, b, word = int(m.group(1)), int(m.group(2)), m.group(3)
                before = line[max(0, m.start() - 12):m.start()]
                if re.search(r"(RM\d+|DS\d+|PM\d+|UM\d+|ES\d+|데이터시트)\s*$", before):
                    continue                     # 제조사 문서의 절 번호
                if a not in written:
                    continue
                title = heads.get((a, b))
                if title is None:
                    print(f"{ch.name}:{n}: {a}.{b}절이 없음")
                    errors += 1
                elif word and VERBOSE:
                    key = re.sub(r"(은|는|이|가|을|를|과|와|에서|으로|로)$", "", word)
                    if key and key not in title:
                        print(f"{ch.name}:{n}: 경고 {a}.{b}절의 '{word}' ↔ 제목 '{title}'")
                        warnings += 1
    print(f"오류 {errors}건, 경고 {warnings}건")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
