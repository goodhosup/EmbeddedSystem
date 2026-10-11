#!/usr/bin/env python3
"""책에 들어가는 SVG 그림을 생성한다.

    python3 tools/figgen.py

을 실행하면 figures/ 아래의 SVG 파일들이 다시 만들어진다.
그림을 고칠 때는 SVG를 직접 고치지 말고 이 스크립트를 고친다.
"""

from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parent.parent
FIG = ROOT / "figures"

FONT = "'Noto Sans KR','Apple SD Gothic Neo','Malgun Gothic',sans-serif"
INK = "#1f2933"
MUTED = "#616e7c"
LINE = "#334e68"
BLUE = "#2f6fdf"
BLUE_BG = "#e3edfd"
ORANGE = "#d9730d"
ORANGE_BG = "#fdebd7"
GREEN = "#2f9e44"
GREEN_BG = "#e1f4e5"
GRAY_BG = "#eef1f4"
RED = "#d64545"


class Svg:
    def __init__(self, width, height):
        self.w, self.h = width, height
        self.items = []

    def add(self, s):
        self.items.append(s)

    def text(self, x, y, s, size=14, anchor="middle", color=INK, weight="normal"):
        self.add(
            f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}" '
            f'fill="{color}" font-weight="{weight}">{escape(s)}</text>'
        )

    def line(self, x1, y1, x2, y2, color=LINE, width=2, dash=None, arrow=False, both=False):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        m = ' marker-end="url(#arrow)"' if arrow or both else ""
        m += ' marker-start="url(#arrow)"' if both else ""
        self.add(
            f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{color}" stroke-width="{width}"{d}{m}/>'
        )

    def rect(self, x, y, w, h, fill=GRAY_BG, stroke=LINE, rx=8, width=1.5):
        self.add(
            f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>'
        )

    def box(self, x, y, w, h, label, sub=None, fill=GRAY_BG, stroke=LINE, size=15):
        self.rect(x, y, w, h, fill, stroke)
        if sub:
            self.text(x + w / 2, y + h / 2 - 3, label, size, weight="bold")
            self.text(x + w / 2, y + h / 2 + 16, sub, 12, color=MUTED)
        else:
            self.text(x + w / 2, y + h / 2 + 5, label, size, weight="bold")

    def poly(self, pts, fill, stroke=LINE, width=1.5):
        p = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
        self.add(f'<polygon points="{p}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')

    def path(self, d, color=LINE, width=2, fill="none", arrow=False):
        m = ' marker-end="url(#arrow)"' if arrow else ""
        self.add(f'<path d="{d}" stroke="{color}" stroke-width="{width}" fill="{fill}" stroke-linejoin="round"{m}/>')

    def save(self, rel):
        out = FIG / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        body = "\n  ".join(self.items)
        out.write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
            f'viewBox="0 0 {self.w} {self.h}" font-family="{FONT}">\n'
            f'  <defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
            f'markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="{LINE}"/>'
            f"</marker></defs>\n"
            f'  <rect width="100%" height="100%" fill="#ffffff"/>\n  {body}\n</svg>\n',
            encoding="utf-8",
        )
        print("wrote", out.relative_to(ROOT))


# ---------------------------------------------------------------------------
# 타이밍 다이어그램
# ---------------------------------------------------------------------------

class Timing:
    """신호를 시간 단위(unit) 칸으로 나누어 그리는 타이밍 다이어그램.

    신호 하나는 (길이, 종류, 라벨, 색) 세그먼트의 목록이다.
      종류 'h' = High, 'l' = Low, 'd' = 데이터(육각형), 'z' = 하이 임피던스/무관
    """

    ROW = 58
    AMP = 26

    def __init__(self, units, unit_w=40, label_w=120, top=50, title=None):
        self.units, self.u, self.lw, self.top = units, unit_w, label_w, top
        self.rows = []
        self.notes = []
        self.title = title

    def x(self, t):
        return self.lw + t * self.u

    def row_y(self, i):
        return self.top + i * self.ROW

    def signal(self, name, segs, sub=None):
        self.rows.append((name, segs, sub))
        return len(self.rows) - 1

    def bits(self, name, levels, sub=None, labels=None):
        """'1'/'0' 문자열을 신호로 만든다. labels가 있으면 칸 위에 글자를 쓴다."""
        segs = [(1, "h" if c == "1" else "l", (labels or {}).get(i), None) for i, c in enumerate(levels)]
        return self.signal(name, segs, sub)

    def clock(self, name, start_level, n_half, lead=0, tail=0, idle=None, sub=None):
        idle = idle or start_level
        segs = []
        if lead:
            segs.append((lead, idle, None, None))
        lvl = start_level
        for _ in range(n_half):
            segs.append((0.5, lvl, None, None))
            lvl = "l" if lvl == "h" else "h"
        if tail:
            segs.append((tail, idle, None, None))
        return self.signal(name, segs, sub)

    def note(self, kind, **kw):
        self.notes.append((kind, kw))

    def render(self, rel, extra_bottom=40):
        h = self.top + len(self.rows) * self.ROW + extra_bottom
        w = self.lw + self.units * self.u + 30
        s = Svg(w, h)
        if self.title:
            s.text(w / 2, 24, self.title, 16, weight="bold")

        # 세로 보조선(점선)은 신호보다 먼저 그린다
        for kind, kw in self.notes:
            if kind == "vline":
                x = self.x(kw["t"])
                y0 = self.row_y(kw.get("from_row", 0)) - 6
                y1 = self.row_y(kw.get("to_row", len(self.rows) - 1)) + self.AMP + 14
                s.line(x, y0, x, y1, kw.get("color", MUTED), 1.2, "4 4")
                if kw.get("label"):
                    s.text(x, y0 - kw.get("dy", 6), kw["label"], 12, color=kw.get("color", MUTED))

        for i, (name, segs, sub) in enumerate(self.rows):
            y = self.row_y(i)
            yh, yl, ym = y, y + self.AMP, y + self.AMP / 2
            s.text(self.lw - 14, ym + (0 if sub else 5), name, 14, "end", weight="bold")
            if sub:
                s.text(self.lw - 14, ym + 17, sub, 11, "end", color=MUTED)
            t = 0.0
            prev = None
            for length, kind, label, color in segs:
                x0, x1 = self.x(t), self.x(t + length)
                k = 6
                if kind in ("h", "l"):
                    yy = yh if kind == "h" else yl
                    if prev in ("h", "l") and prev != kind:
                        s.line(x0, yh, x0, yl, color or LINE, 2)
                    elif prev in ("d", "z"):
                        s.line(x0 - k, ym, x0, yy, color or LINE, 2)
                    s.line(x0, yy, x1, yy, color or LINE, 2)
                    if label:
                        s.text((x0 + x1) / 2, yh - 6, label, 12, color=MUTED)
                elif kind == "d":
                    fill = color or BLUE_BG
                    if prev in ("h", "l"):
                        py = yh if prev == "h" else yl
                        s.line(x0, py, x0 + k, yh if prev == "h" else yl, LINE, 2)
                    s.poly([(x0, ym), (x0 + k, yh), (x1 - k, yh), (x1, ym), (x1 - k, yl), (x0 + k, yl)], fill)
                    if label:
                        s.text((x0 + x1) / 2, ym + 5, label, 13, weight="bold")
                elif kind == "z":
                    s.line(x0, ym, x1, ym, MUTED, 2, "6 4")
                    if label:
                        s.text((x0 + x1) / 2, yh - 4, label, 12, color=MUTED)
                prev = kind
                t += length

        for kind, kw in self.notes:
            if kind == "span":
                # 신호 아래(또는 위)에 구간 괄호와 설명
                r = kw["row"]
                below = kw.get("below", True)
                x0, x1 = self.x(kw["t0"]), self.x(kw["t1"])
                yb = self.row_y(r) + (self.AMP + 12 if below else -12)
                d = 5 if below else -5
                col = kw.get("color", ORANGE)
                s.path(f"M{x0+2:.1f},{yb-d:.1f} L{x0+2:.1f},{yb:.1f} L{x1-2:.1f},{yb:.1f} L{x1-2:.1f},{yb-d:.1f}", col, 1.5)
                s.text((x0 + x1) / 2, yb + (16 if below else -6), kw["label"], 12, color=col, weight="bold")
            elif kind == "sample":
                # 샘플링 시점 표시(작은 삼각형)
                r = kw["row"]
                col = kw.get("color", RED)
                for t in kw["ts"]:
                    x = self.x(t)
                    y = self.row_y(r) + self.AMP + 6
                    s.poly([(x, y), (x - 5, y + 9), (x + 5, y + 9)], col, col, 1)
            elif kind == "text":
                s.text(self.x(kw["t"]), kw["y"], kw["label"], kw.get("size", 12),
                       kw.get("anchor", "middle"), kw.get("color", MUTED))
        s.save(rel)


# ---------------------------------------------------------------------------
# 1장
# ---------------------------------------------------------------------------

def fig_1_1():
    s = Svg(820, 400)
    s.text(410, 30, "그림 1-1  임베디드 시스템의 구성 요소", 16, weight="bold")
    s.rect(40, 50, 740, 330, "#fafbfc", MUTED, 14, 1.2)
    s.text(70, 75, "디지털 온도 조절기의 예", 13, "start", MUTED)

    # 입력
    s.box(70, 120, 150, 60, "센서", "온도 · 습도 센서", GREEN_BG, GREEN)
    s.box(70, 210, 150, 60, "사용자 입력", "버튼, 다이얼", GREEN_BG, GREEN)
    # MCU
    s.rect(290, 100, 240, 190, BLUE_BG, BLUE, 12, 2)
    s.text(410, 128, "마이크로컨트롤러 (MCU)", 15, weight="bold")
    for i, (a, b) in enumerate([("CPU 코어", "명령 실행"), ("메모리", "Flash · SRAM"), ("주변장치", "GPIO · ADC · UART")]):
        s.rect(305, 142 + i * 47, 210, 40, "#ffffff", BLUE, 6)
        s.text(318, 142 + i * 47 + 25, a, 13, "start", weight="bold")
        s.text(503, 142 + i * 47 + 25, b, 11, "end", MUTED)
    # 출력
    s.box(600, 120, 150, 60, "액추에이터", "히터 릴레이, 모터", ORANGE_BG, ORANGE)
    s.box(600, 210, 150, 60, "표시 장치", "LCD, LED", ORANGE_BG, ORANGE)
    for y in (150, 240):
        s.line(222, y, 286, y, arrow=True)
        s.line(532, y, 596, y, arrow=True)
    # 전원 / 통신
    s.box(260, 315, 140, 50, "전원", "배터리 · 레귤레이터")
    s.box(420, 315, 140, 50, "통신", "UART · Wi-Fi · BLE")
    s.line(330, 313, 330, 294, arrow=True)
    s.line(490, 311, 490, 294, both=True)
    s.text(145, 105, "입력", 13, color=GREEN, weight="bold")
    s.text(675, 105, "출력", 13, color=ORANGE, weight="bold")
    s.save("ch01/fig1-1-components.svg")


def fig_1_2():
    s = Svg(820, 330)
    s.text(410, 30, "그림 1-2  MCU와 MPU 시스템 비교", 16, weight="bold")
    # MCU
    s.text(205, 62, "MCU: 칩 하나에 거의 모든 것", 14, weight="bold", color=BLUE)
    s.rect(60, 75, 290, 200, BLUE_BG, BLUE, 12, 2)
    s.text(205, 98, "STM32F411 (칩 1개)", 13, color=MUTED)
    for i, (lbl, sub) in enumerate([("CPU", "100 MHz"), ("Flash", "512 KB"), ("SRAM", "128 KB"), ("주변장치", "GPIO, ADC, ...")]):
        x = 80 + (i % 2) * 130
        y = 112 + (i // 2) * 75
        s.box(x, y, 120, 60, lbl, sub, "#ffffff", BLUE, 14)
    s.text(205, 300, "운영체제 없이(베어메탈) 또는 RTOS · 수 mA · 즉시 부팅", 12, color=MUTED)
    # MPU
    s.text(615, 62, "MPU: 외부 메모리와 운영체제", 14, weight="bold", color=ORANGE)
    s.rect(470, 75, 290, 200, "#fafbfc", MUTED, 12, 1.2)
    s.text(615, 98, "보드 위의 여러 칩", 13, color=MUTED)
    s.box(490, 112, 120, 60, "SoC", "CPU 1.5 GHz", ORANGE_BG, ORANGE, 14)
    s.box(630, 112, 110, 60, "DRAM", "4 GB", "#ffffff", ORANGE, 14)
    s.box(490, 187, 120, 60, "저장장치", "eMMC · SD", "#ffffff", ORANGE, 14)
    s.box(630, 187, 110, 60, "PMIC", "전원 관리", "#ffffff", ORANGE, 14)
    s.line(612, 142, 628, 142, ORANGE)
    s.line(550, 174, 550, 185, ORANGE)
    s.text(615, 300, "리눅스 같은 범용 OS · 수백 mA 이상 · 부팅 수 초", 12, color=MUTED)
    s.save("ch01/fig1-2-mcu-vs-mpu.svg")


def fig_1_3():
    s = Svg(820, 360)
    s.text(410, 30, "그림 1-3  하드 실시간과 소프트 실시간", 16, weight="bold")
    x0, x1, dl = 120, 760, 470

    def axis(y, title):
        s.text(x0 - 40, y - 100, title, 14, "start", weight="bold")
        s.line(x0, y, x1, y, MUTED, 1.5, arrow=True)
        s.line(x0, y, x0, y - 75, MUTED, 1.5, arrow=True)
        s.text(x0 - 8, y - 62, "가치", 12, "end", MUTED)
        s.text(x1, y + 18, "응답 시간", 12, "end", MUTED)
        s.line(dl, y - 75, dl, y + 6, RED, 1.5, "5 4")
        s.text(dl, y + 20, "마감 시간", 12, color=RED, weight="bold")

    axis(160, "하드 실시간: 늦으면 실패 (에어백, 모터 제어)")
    s.path(f"M{x0},{100} L{dl},{100} L{dl},{160} L{x1 - 10},{160}", BLUE, 3)
    s.text(620, 145, "가치 0 또는 재난", 12, color=BLUE)

    axis(320, "소프트 실시간: 늦을수록 품질 저하 (영상 재생, UI)")
    s.path(f"M{x0},{260} L{dl},{260} Q{dl + 90},{262} {dl + 150},{305} L{x1 - 10},{318}", ORANGE, 3)
    s.text(650, 280, "점점 나빠짐", 12, color=ORANGE)
    s.save("ch01/fig1-3-realtime.svg")


def fig_1_4():
    s = Svg(860, 250)
    s.text(430, 30, "그림 1-4  펌웨어 개발 흐름", 16, weight="bold")
    steps = [("코드 작성", "C 소스 · PC"), ("크로스 컴파일", "avr-gcc · arm-none-eabi-gcc"),
             ("플래싱", "부트로더 · ISP · ST-LINK"), ("실행 · 디버깅", "보드 위에서 동작")]
    for i, (a, b) in enumerate(steps):
        x = 40 + i * 205
        fill, st = (GREEN_BG, GREEN) if i < 2 else (BLUE_BG, BLUE)
        s.box(x, 70, 170, 70, a, b, fill, st)
        if i < 3:
            s.line(x + 172, 105, x + 203, 105, arrow=True)
    s.path("M740,142 L740,190 L125,190 L125,146", MUTED, 1.5, arrow=True)
    s.text(432, 210, "수정하고 다시 빌드 (이 반복이 하루에도 수십 번 일어납니다)", 12, color=MUTED)
    s.text(245, 60, "개발용 PC (호스트)", 12, color=GREEN)
    s.text(655, 60, "타깃 보드", 12, color=BLUE)
    s.save("ch01/fig1-4-workflow.svg")


def fig_1_5():
    s = Svg(880, 330)
    s.text(440, 30, "그림 1-5  이 책의 실습 보드 네 가지와 역할", 16, weight="bold")
    boards = [
        ("아두이노 우노", "ATmega328P · AVR 8비트", GREEN_BG, GREEN, ["입문", "아두이노 함수 → 레지스터", "6~16장 AVR 구현"]),
        ("ATmega128", "ATmega128A · AVR 8비트", GRAY_BG, LINE, ["우노와 다른 점 박스", "포트 A~G, UART 2개", "ISP 프로그래밍"]),
        ("STM32", "NUCLEO-F411RE · Cortex-M4F", BLUE_BG, BLUE, ["6~16장 32비트 구현", "DMA · RTOS · 부트로더", "보안 · 최적화 · 양산"]),
        ("ESP32", "ESP32-DevKitC · Wi-Fi/BLE", ORANGE_BG, ORANGE, ["무선 · IoT", "아두이노 IDE → ESP-IDF", "OTA · 최종 프로젝트"]),
    ]
    for i, (name, sub, fill, st, roles) in enumerate(boards):
        x = 30 + i * 210
        s.box(x, 60, 190, 70, name, sub, fill, st)
        for j, r in enumerate(roles):
            s.rect(x + 10, 150 + j * 40, 170, 32, "#ffffff", st, 6, 1.2)
            s.text(x + 95, 171 + j * 40, r, 12)
    s.line(60, 290, 820, 290, MUTED, 1.5, arrow=True)
    s.text(60, 312, "8비트, 단순함", 12, "start", MUTED)
    s.text(820, 312, "32비트, 고성능 · 무선", 12, "end", MUTED)
    s.save("ch01/fig1-5-boards.svg")


def fig_1_6():
    s = Svg(860, 300)
    s.text(430, 30, "그림 1-6  digitalWrite(13, HIGH) 한 줄이 실제로 하는 일", 16, weight="bold")
    s.box(40, 70, 230, 60, "digitalWrite(13, HIGH)", "아두이노 함수", GREEN_BG, GREEN, 14)
    s.line(272, 100, 318, 100, arrow=True)
    s.text(295, 62, "핀 번호 변환", 11, color=MUTED)
    s.box(320, 70, 200, 60, "13번 핀 = PB5", "포트 B의 5번 비트", GRAY_BG, LINE, 14)
    s.line(522, 100, 568, 100, arrow=True)
    s.box(570, 70, 250, 60, "PORTB |= (1 << 5)", "레지스터에 비트 쓰기", BLUE_BG, BLUE, 14)
    # PORTB 레지스터 비트
    s.text(605, 211, "← PORTB 레지스터 (8비트)", 13, "start", weight="bold")
    for i in range(8):
        bit = 7 - i
        on = bit == 5
        x = 270 + i * 40
        s.rect(x, 185, 40, 40, ORANGE_BG if on else "#ffffff", ORANGE if on else MUTED, 2)
        s.text(x + 20, 211, "1" if on else "0", 15, weight="bold")
        s.text(x + 20, 242, str(bit), 11, color=MUTED)
    s.path("M695,133 L695,155 L370,155 L370,181", ORANGE, 1.5, arrow=True)
    s.text(430, 275, "5번 비트가 1이 되는 순간 PB5 핀의 전압이 5V가 되고, 연결된 LED가 켜진다", 12, color=MUTED)
    s.save("ch01/fig1-6-arduino-vs-register.svg")


# ---------------------------------------------------------------------------
# 회로 기호 (2장 이후 공통)
# ---------------------------------------------------------------------------

def resistor_v(s, x, y0, y1, label=None, color=LINE):
    """세로 저항. y0~y1 사이 가운데에 지그재그를 그린다."""
    mid = (y0 + y1) / 2
    zig_h = 40
    a, b = mid - zig_h / 2, mid + zig_h / 2
    s.line(x, y0, x, a, color, 2)
    pts = [f"M{x:.1f},{a:.1f}"]
    n = 6
    for i in range(n):
        dx = 8 if i % 2 == 0 else -8
        pts.append(f"L{x + dx:.1f},{a + (i + 0.5) * zig_h / n:.1f}")
    pts.append(f"L{x:.1f},{b:.1f}")
    s.path(" ".join(pts), color, 2)
    s.line(x, b, x, y1, color, 2)
    if label:
        s.text(x + 16, mid + 5, label, 13, "start", weight="bold")


def resistor_h(s, x0, x1, y, label=None, color=LINE):
    """가로 저항."""
    mid = (x0 + x1) / 2
    zig_w = 40
    a, b = mid - zig_w / 2, mid + zig_w / 2
    s.line(x0, y, a, y, color, 2)
    pts = [f"M{a:.1f},{y:.1f}"]
    n = 6
    for i in range(n):
        dy = -8 if i % 2 == 0 else 8
        pts.append(f"L{a + (i + 0.5) * zig_w / n:.1f},{y + dy:.1f}")
    pts.append(f"L{b:.1f},{y:.1f}")
    s.path(" ".join(pts), color, 2)
    s.line(b, y, x1, y, color, 2)
    if label:
        s.text(mid, y - 16, label, 13, weight="bold")


def led_v(s, x, y0, y1, label=None, color=RED):
    """세로 LED (위가 애노드, 아래가 캐소드)."""
    mid = (y0 + y1) / 2
    s.line(x, y0, x, mid - 12, LINE, 2)
    s.poly([(x - 12, mid - 12), (x + 12, mid - 12), (x, mid + 8)], "#ffffff", color, 2)
    s.line(x - 12, mid + 8, x + 12, mid + 8, color, 2.5)
    s.line(x, mid + 8, x, y1, LINE, 2)
    for k in (0, 9):
        s.line(x + 16, mid - 8 + k, x + 27, mid - 16 + k, color, 1.5)
        s.poly([(x + 27, mid - 16 + k), (x + 21, mid - 15 + k), (x + 25, mid - 11 + k)], color, color, 1)
    if label:
        s.text(x + 34, mid + 5, label, 13, "start", weight="bold")


def ground(s, x, y, color=LINE):
    s.line(x, y, x, y + 8, color, 2)
    for i, w in enumerate((14, 9, 4)):
        s.line(x - w, y + 8 + i * 5, x + w, y + 8 + i * 5, color, 2)


def supply(s, x, y, label, color=LINE):
    """전원 기호: 위쪽 가로 막대와 전압 이름."""
    s.line(x - 14, y, x + 14, y, color, 2.5)
    s.text(x, y - 8, label, 13, weight="bold")


def switch_v(s, x, y0, y1, label=None, closed=False, color=LINE):
    """세로 푸시 버튼."""
    mid = (y0 + y1) / 2
    s.line(x, y0, x, mid - 12, color, 2)
    s.line(x, mid + 12, x, y1, color, 2)
    s.add(f'<circle cx="{x:.1f}" cy="{mid - 12:.1f}" r="3" fill="{color}"/>')
    s.add(f'<circle cx="{x:.1f}" cy="{mid + 12:.1f}" r="3" fill="{color}"/>')
    if closed:
        s.line(x - 4, mid - 12, x - 4, mid + 12, color, 2.5)
    else:
        s.line(x - 14, mid - 16, x - 14, mid + 16, color, 2.5)
        s.line(x - 14, mid, x - 26, mid, color, 2)
    if label:
        s.text(x + 14, mid + 5, label, 13, "start", weight="bold")


def dot(s, x, y, color=LINE):
    s.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{color}"/>')


# ---------------------------------------------------------------------------
# 2장
# ---------------------------------------------------------------------------

def fig_2_1():
    s = Svg(820, 380)
    s.text(410, 30, "그림 2-1  옴의 법칙과 LED 회로", 16, weight="bold")
    # 왼쪽: 옴의 법칙 삼각형
    s.text(190, 70, "옴의 법칙", 14, weight="bold", color=BLUE)
    s.poly([(190, 95), (90, 265), (290, 265)], BLUE_BG, BLUE, 2)
    s.line(125, 205, 255, 205, BLUE, 2)
    s.line(190, 205, 190, 265, BLUE, 2)
    s.text(190, 180, "V", 30, weight="bold")
    s.text(155, 248, "I", 28, weight="bold")
    s.text(225, 248, "R", 28, weight="bold")
    s.text(190, 300, "V = I × R", 16, weight="bold")
    s.text(190, 324, "I = V ÷ R,   R = V ÷ I", 13, color=MUTED)
    s.text(190, 346, "가리고 싶은 값을 손으로 덮으면 식이 보인다", 12, color=MUTED)

    # 오른쪽: LED 회로
    s.text(590, 70, "LED 전류 제한 저항", 14, weight="bold", color=ORANGE)
    x = 520
    s.box(400, 95, 90, 44, "MCU 핀", None, BLUE_BG, BLUE, 13)
    s.text(445, 155, "High = 5 V", 12, color=BLUE)
    s.line(490, 117, x, 117, LINE, 2)
    s.line(x, 117, x, 120, LINE, 2)
    resistor_v(s, x, 120, 200, "R = 330 Ω")
    led_v(s, x, 200, 275, "빨간 LED")
    ground(s, x, 275)
    # 전압 표시
    s.line(650, 128, 650, 192, ORANGE, 1.5, both=True)
    s.text(660, 165, "3 V", 13, "start", ORANGE, "bold")
    s.line(650, 208, 650, 268, RED, 1.5, both=True)
    s.text(660, 243, "Vf ≈ 2 V", 13, "start", RED, "bold")
    s.rect(400, 310, 380, 52, GRAY_BG, MUTED, 8, 1)
    s.text(590, 332, "저항에 걸리는 전압 = 5 V − 2 V = 3 V", 13)
    s.text(590, 352, "I = 3 V ÷ 330 Ω ≈ 9 mA (핀 허용 전류 안쪽)", 13, weight="bold")
    s.save("ch02/fig2-1-ohm-led.svg")


def fig_2_2():
    s = Svg(820, 420)
    s.text(410, 30, "그림 2-2  논리 레벨과 노이즈 마진 (ATmega328P, VCC = 5 V 기준)", 16, weight="bold")
    base, k = 360, 56  # 0 V의 y좌표, 1 V당 픽셀

    def y(v):
        return base - v * k

    # 전압 축
    s.line(70, y(0), 70, y(5.3), MUTED, 1.5, arrow=True)
    for v in range(6):
        s.line(64, y(v), 70, y(v), MUTED, 1.5)
        s.text(58, y(v) + 4, f"{v} V", 12, "end", MUTED)

    # 출력(송신) 막대
    xo = 200
    s.text(xo + 60, 62, "출력이 보장하는 값", 13, weight="bold", color=BLUE)
    s.rect(xo, y(5), 120, y(4.2) - y(5), BLUE_BG, BLUE, 3)
    s.text(xo + 60, (y(5) + y(4.2)) / 2 + 5, "High 출력", 13, weight="bold")
    s.rect(xo, y(0.9), 120, y(0) - y(0.9), BLUE_BG, BLUE, 3)
    s.text(xo + 60, (y(0.9) + y(0)) / 2 + 5, "Low 출력", 13, weight="bold")
    s.text(xo - 6, y(4.2) + 4, "VOH min 4.2 V", 11, "end", BLUE)
    s.text(xo - 6, y(0.9) + 4, "VOL max 0.9 V", 11, "end", BLUE)

    # 입력(수신) 막대
    xi = 520
    s.text(xi + 60, 62, "입력이 받아들이는 값", 13, weight="bold", color=ORANGE)
    s.rect(xi, y(5), 120, y(3.0) - y(5), ORANGE_BG, ORANGE, 3)
    s.text(xi + 60, (y(5) + y(3.0)) / 2 + 5, "High로 인식", 13, weight="bold")
    s.rect(xi, y(3.0), 120, y(1.5) - y(3.0), "#ffffff", RED, 3)
    s.text(xi + 60, (y(3.0) + y(1.5)) / 2 - 4, "정의되지 않음", 13, color=RED, weight="bold")
    s.text(xi + 60, (y(3.0) + y(1.5)) / 2 + 14, "(피해야 하는 구간)", 11, color=RED)
    s.rect(xi, y(1.5), 120, y(0) - y(1.5), ORANGE_BG, ORANGE, 3)
    s.text(xi + 60, (y(1.5) + y(0)) / 2 + 5, "Low로 인식", 13, weight="bold")
    s.text(xi + 126, y(3.0) + 4, "VIH min 3.0 V", 11, "start", ORANGE)
    s.text(xi + 126, y(1.5) + 4, "VIL max 1.5 V", 11, "start", ORANGE)

    # 노이즈 마진
    for va, vb, lbl in ((4.2, 3.0, "High 노이즈 마진 1.2 V"), (1.5, 0.9, "Low 노이즈 마진 0.6 V")):
        s.line(330, y(va), 510, y(va), GREEN, 1.2, "4 3")
        s.line(330, y(vb), 510, y(vb), GREEN, 1.2, "4 3")
        s.line(360, y(va) + 1, 360, y(vb) - 1, GREEN, 1.5, both=True)
        s.text(372, (y(va) + y(vb)) / 2 + 5, lbl, 12, "start", GREEN, "bold")
    s.text(410, 400, "출력의 최악값과 입력의 경계 사이 여유(노이즈 마진)만큼 잡음을 견딜 수 있다", 12, color=MUTED)
    s.save("ch02/fig2-2-logic-levels.svg")


def fig_2_3():
    s = Svg(820, 400)
    s.text(410, 30, "그림 2-3  풀업과 풀다운 저항이 있는 버튼 회로", 16, weight="bold")

    def mcu(x, y):
        s.box(x, y - 22, 100, 44, "입력 핀", None, BLUE_BG, BLUE, 13)

    # (가) 풀업
    s.text(205, 64, "(가) 풀업: 평소 High, 누르면 Low", 14, weight="bold", color=BLUE)
    x = 150
    supply(s, x, 100, "VCC")
    resistor_v(s, x, 100, 180, "10 kΩ")
    dot(s, x, 200)
    s.line(x, 180, x, 280, LINE, 2)
    s.line(x, 200, 250, 200, LINE, 2)
    mcu(250, 200)
    switch_v(s, x, 230, 300, "버튼")
    ground(s, x, 300)
    s.rect(60, 340, 290, 46, GRAY_BG, MUTED, 8, 1)
    s.text(205, 359, "안 누름: 저항을 통해 VCC → 1", 12)
    s.text(205, 377, "누름: 핀이 GND에 직접 연결 → 0", 12)

    # (나) 풀다운
    s.text(615, 64, "(나) 풀다운: 평소 Low, 누르면 High", 14, weight="bold", color=ORANGE)
    x = 560
    supply(s, x, 100, "VCC")
    switch_v(s, x, 100, 170, "버튼")
    dot(s, x, 200)
    s.line(x, 170, x, 220, LINE, 2)
    s.line(x, 200, 660, 200, LINE, 2)
    mcu(660, 200)
    resistor_v(s, x, 220, 300, "10 kΩ")
    ground(s, x, 300)
    s.rect(470, 340, 290, 46, GRAY_BG, MUTED, 8, 1)
    s.text(615, 359, "안 누름: 저항을 통해 GND → 0", 12)
    s.text(615, 377, "누름: 핀이 VCC에 직접 연결 → 1", 12)
    s.save("ch02/fig2-3-pullup-pulldown.svg")


def fig_2_4():
    s = Svg(820, 420)
    s.text(410, 30, "그림 2-4  푸시풀 출력과 오픈 드레인 출력의 내부 구조", 16, weight="bold")

    def fet(x, y, name, on, color):
        """트랜지스터를 스위치 상자로 단순화해서 그린다."""
        s.rect(x - 45, y - 24, 90, 48, color if on else "#ffffff", LINE, 6, 1.5)
        s.text(x, y - 3, name, 13, weight="bold")
        s.text(x, y + 15, "켜짐(ON)" if on else "꺼짐(OFF)", 11, color=INK if on else MUTED)

    # (가) 푸시풀
    s.text(210, 64, "(가) 푸시풀: 1과 0을 모두 힘 있게 낸다", 14, weight="bold", color=BLUE)
    x = 150
    supply(s, x, 100, "VDD")
    s.line(x, 100, x, 126, LINE, 2)
    fet(x, 150, "P-MOS", True, BLUE_BG)
    s.line(x, 174, x, 236, BLUE, 3)
    dot(s, x, 205)
    s.line(x, 205, 250, 205, BLUE, 3, arrow=True)
    s.text(258, 210, "출력 = 1", 13, "start", BLUE, "bold")
    fet(x, 260, "N-MOS", False, BLUE_BG)
    s.line(x, 284, x, 300, LINE, 2)
    ground(s, x, 300)
    s.text(150, 350, "출력 1: 위가 켜져 VDD로 끌어올림", 12, color=MUTED)
    s.text(150, 370, "출력 0: 아래가 켜져 GND로 끌어내림", 12, color=MUTED)
    s.text(150, 390, "두 출력을 서로 연결하면 단락 위험", 12, color=RED, weight="bold")

    # (나) 오픈 드레인
    s.text(610, 64, "(나) 오픈 드레인: 0만 낸다", 14, weight="bold", color=ORANGE)
    x = 560
    s.rect(500, 110, 120, 200, "#fafbfc", MUTED, 10, 1)
    s.text(560, 128, "칩 내부", 11, color=MUTED)
    s.text(560, 172, "(위쪽 트랜지스터 없음)", 11, color=MUTED)
    s.line(x, 205, x, 236, LINE, 2)
    dot(s, x, 205)
    fet(x, 260, "N-MOS", False, ORANGE_BG)
    s.line(x, 284, x, 300, LINE, 2)
    ground(s, x, 300)
    s.line(x, 205, 700, 205, ORANGE, 3)
    supply(s, 700, 100, "VDD")
    resistor_v(s, 700, 100, 180, "외부 풀업")
    s.line(700, 180, 700, 205, ORANGE, 3)
    dot(s, 700, 205)
    s.line(700, 205, 760, 205, ORANGE, 3, arrow=True)
    s.text(730, 230, "출력 = 1", 13, color=ORANGE, weight="bold")
    s.text(610, 350, "0: 트랜지스터가 켜져 GND로 끌어내림", 12, color=MUTED)
    s.text(610, 370, "1: 트랜지스터가 꺼지고 풀업 저항이 끌어올림", 12, color=MUTED)
    s.text(610, 390, "여러 출력을 한 선에 묶어도 안전 (I2C)", 12, color=GREEN, weight="bold")
    s.save("ch02/fig2-4-pushpull-opendrain.svg")


def fig_2_5():
    s = Svg(820, 500)
    s.text(410, 30, "그림 2-5  브레드보드 내부 연결 구조", 16, weight="bold")
    x0, y0, p = 110, 70, 26          # 왼쪽 위 구멍 위치, 구멍 간격
    cols = 22
    s.rect(70, 50, 690, 362, "#fbfaf6", MUTED, 10, 1.2)

    def hole(x, y):
        s.rect(x - 5, y - 5, 10, 10, "#ffffff", MUTED, 2, 1)

    # 전원 레일 (위)
    rails = [(y0, "+", RED), (y0 + p, "−", BLUE)]
    for y, sign, col in rails:
        s.line(x0 - 10, y, x0 + (cols - 1) * p + 10, y, col, 5)
        for c in range(cols):
            hole(x0 + c * p, y)
        s.text(x0 - 22, y + 5, sign, 16, color=col, weight="bold")
    # 단자 줄 a~e, f~j
    top_rows = "abcde"
    bot_rows = "fghij"
    ya = y0 + 2.3 * p
    yf = ya + 5 * p + 0.9 * p
    hl = {3: GREEN, 4: GREEN, 9: ORANGE}
    for c in range(cols):
        x = x0 + c * p
        col = hl.get(c)
        if col:
            s.line(x, ya, x, ya + 4 * p, col, 6)
            s.line(x, yf, x, yf + 4 * p, col, 6)
        for r in range(5):
            hole(x, ya + r * p)
            hole(x, yf + r * p)
    for r in range(5):
        s.text(x0 - 22, ya + r * p + 4, top_rows[r], 12, color=MUTED)
        s.text(x0 - 22, yf + r * p + 4, bot_rows[r], 12, color=MUTED)
    s.rect(x0 - 10, ya + 4 * p + 10, (cols - 1) * p + 20, yf - ya - 4 * p - 20, "#e9e6dc", MUTED, 3, 1)
    s.text(x0 + 11 * p, (ya + 4 * p + yf) / 2 + 4, "가운데 홈: 위아래는 연결되지 않는다 (IC를 걸쳐 꽂는 자리)", 11, color=MUTED)

    # 설명
    legend = [
        (RED, "전원 레일(+, −): 가로 한 줄 전체가 연결된다. 긴 보드는 가운데에서 끊긴 제품도 있다"),
        (GREEN, "단자 줄: 세로 다섯 칸(a~e 또는 f~j)이 한 묶음이다"),
        (ORANGE, "같은 열이라도 위(a~e)와 아래(f~j)는 서로 다른 묶음이다"),
    ]
    for i, (col, txt) in enumerate(legend):
        yy = 438 + i * 22
        s.line(90, yy - 4, 120, yy - 4, col, 5)
        s.text(132, yy, txt, 12, "start", INK)
    s.save("ch02/fig2-5-breadboard.svg")


def fig_2_6():
    s = Svg(860, 435)
    s.text(430, 30, "그림 2-6  데이터시트의 전기적 특성 표 읽는 법 (예시 표)", 16, weight="bold")
    headers = ["기호", "파라미터", "조건", "Min", "Typ", "Max", "단위"]
    widths = [70, 190, 150, 60, 60, 60, 60]
    rows = [
        ["VIL", "입력 Low 전압", "VCC = 5 V", "−0.5", "", "0.3VCC", "V"],
        ["VIH", "입력 High 전압", "VCC = 5 V", "0.6VCC", "", "VCC+0.5", "V"],
        ["VOL", "출력 Low 전압", "IOL = 20 mA", "", "", "0.9", "V"],
        ["VOH", "출력 High 전압", "IOH = −20 mA", "4.2", "", "", "V"],
        ["RPU", "내부 풀업 저항", "", "20", "", "50", "kΩ"],
    ]
    x0, y0, rh = 70, 70, 36
    x = x0
    for h, w in zip(headers, widths):
        s.rect(x, y0, w, rh, BLUE_BG, LINE, 0, 1)
        s.text(x + w / 2, y0 + 23, h, 13, weight="bold")
        x += w
    for r, row in enumerate(rows):
        x = x0
        y = y0 + (r + 1) * rh
        for c, (v, w) in enumerate(zip(row, widths)):
            fill = ORANGE_BG if c == 2 and v else "#ffffff"
            s.rect(x, y, w, rh, fill, LINE, 0, 1)
            s.text(x + w / 2, y + 23, v, 12, weight="bold" if c == 0 else "normal")
            x += w
    yb = y0 + 6 * rh
    # 번호 배지(표 머리 위)와 해설
    notes = [
        (x0 + 35, "①", "기호: 본문과 그림에서 이 이름으로 부른다", BLUE),
        (x0 + 260 + 75, "②", "조건: 이 조건에서만 표의 값이 보장된다", ORANGE),
        (x0 + 410 + 90, "③", "Min/Max는 보장값, Typ는 참고값이다. 설계는 최악값(Min/Max)으로 한다", GREEN),
        (x0 + 560 + 30, "④", "단위와 부호를 확인한다. 전류의 −는 핀에서 흘러나가는 방향이다", RED),
    ]
    for i, (nx, badge, txt, col) in enumerate(notes):
        s.add(f'<circle cx="{nx:.1f}" cy="{y0 - 14:.1f}" r="11" fill="{col}"/>')
        s.text(nx, y0 - 9, str(i + 1), 13, color="#ffffff", weight="bold")
        ty = yb + 32 + i * 26
        s.add(f'<circle cx="{x0 + 10:.1f}" cy="{ty - 5:.1f}" r="10" fill="{col}"/>')
        s.text(x0 + 10, ty, str(i + 1), 12, color="#ffffff", weight="bold")
        s.text(x0 + 30, ty, txt, 13, "start", INK)
    s.text(430, 418, "값은 ATmega328P 데이터시트의 DC 특성 표를 단순화한 것이다. 실제 설계에는 원본 표를 확인한다.", 11, color=MUTED)
    s.save("ch02/fig2-6-datasheet-table.svg")


# ---------------------------------------------------------------------------
# 3장
# ---------------------------------------------------------------------------

def fig_3_1():
    s = Svg(820, 420)
    s.text(410, 30, "그림 3-1  ATmega328P 블록 다이어그램 (단순화)", 16, weight="bold")
    s.rect(30, 50, 760, 350, "#fafbfc", MUTED, 14, 1.2)
    # 코어
    s.rect(60, 80, 250, 190, BLUE_BG, BLUE, 10, 2)
    s.text(185, 104, "AVR CPU 코어 (8비트)", 14, weight="bold")
    s.box(75, 118, 105, 54, "ALU", "8비트 연산", "#ffffff", BLUE, 13)
    s.box(190, 118, 105, 54, "R0~R31", "범용 레지스터", "#ffffff", BLUE, 13)
    s.box(75, 184, 105, 54, "PC · SP", "SREG", "#ffffff", BLUE, 13)
    s.box(190, 184, 105, 54, "명령 디코더", None, "#ffffff", BLUE, 12)
    s.text(185, 258, "대부분의 명령을 1클럭에 실행", 11, color=MUTED)
    # 메모리
    s.box(350, 80, 130, 56, "Flash", "32 KB · 프로그램", GREEN_BG, GREEN, 14)
    s.box(350, 147, 130, 56, "SRAM", "2 KB · 데이터", GREEN_BG, GREEN, 14)
    s.box(350, 214, 130, 56, "EEPROM", "1 KB · 설정 저장", GREEN_BG, GREEN, 14)
    s.line(312, 108, 346, 108, GREEN, 2, arrow=True)
    s.text(329, 100, "명령", 10, color=GREEN)
    s.line(312, 175, 346, 175, GREEN, 2, both=True)
    s.line(312, 242, 346, 242, GREEN, 2, both=True)
    # 데이터 버스
    s.rect(60, 292, 700, 26, GRAY_BG, LINE, 6, 1.5)
    s.text(410, 310, "8비트 데이터 버스", 12, weight="bold")
    s.line(185, 272, 185, 290, LINE, 2, both=True)
    s.line(415, 272, 415, 290, LINE, 2, both=True)
    # 주변장치
    per = [("GPIO", "포트 B·C·D"), ("타이머", "0 · 1 · 2"), ("USART", "UART 1개"), ("SPI", ""),
           ("TWI", "I2C"), ("ADC", "10비트"), ("WDT", "워치독")]
    for i, (a, b) in enumerate(per):
        x = 520 + (i % 2) * 125
        y = 80 + (i // 2) * 52
        s.box(x, y, 115, 44, a, b or None, ORANGE_BG, ORANGE, 13)
    s.line(640, 290, 640, 274, LINE, 2, both=True)
    # 클럭/리셋
    s.box(60, 334, 220, 50, "클럭", "16 MHz 크리스털 (우노)", GRAY_BG, LINE, 13)
    s.box(300, 334, 220, 50, "리셋 · 전원 감시", "POR · BOD · 외부 · WDT", GRAY_BG, LINE, 13)
    s.box(540, 334, 220, 50, "인터럽트 유닛", "벡터 26개", GRAY_BG, LINE, 13)
    s.save("ch03/fig3-1-atmega328p-block.svg")


def fig_3_2():
    s = Svg(860, 470)
    s.text(430, 30, "그림 3-2  STM32F411 블록 다이어그램 (단순화)", 16, weight="bold")
    s.rect(30, 50, 800, 400, "#fafbfc", MUTED, 14, 1.2)
    # 코어
    s.rect(55, 75, 230, 150, BLUE_BG, BLUE, 10, 2)
    s.text(170, 98, "Arm Cortex-M4F (32비트)", 14, weight="bold")
    s.box(70, 110, 95, 46, "CPU", "최대 100 MHz", "#ffffff", BLUE, 13)
    s.box(175, 110, 95, 46, "FPU", "단정밀도", "#ffffff", BLUE, 13)
    s.box(70, 166, 95, 46, "NVIC", "인터럽트", "#ffffff", BLUE, 13)
    s.box(175, 166, 95, 46, "SysTick", "MPU · 디버그", "#ffffff", BLUE, 12)
    # 버스 매트릭스
    s.rect(320, 75, 60, 360, GRAY_BG, LINE, 8, 2)
    for i, ch in enumerate("버스매트릭스"):
        s.text(350, 160 + i * 22, ch, 14, weight="bold")
    s.line(287, 130, 316, 130, LINE, 2.5, both=True)
    s.line(287, 190, 316, 190, LINE, 2.5, both=True)
    # DMA
    s.box(55, 250, 230, 50, "DMA1 · DMA2", "CPU 대신 데이터 복사", ORANGE_BG, ORANGE, 13)
    s.line(287, 275, 316, 275, LINE, 2.5, both=True)
    # 메모리
    s.box(415, 75, 170, 56, "Flash", "512 KB · 0x0800 0000", GREEN_BG, GREEN, 14)
    s.box(415, 141, 170, 56, "SRAM", "128 KB · 0x2000 0000", GREEN_BG, GREEN, 14)
    s.line(382, 103, 411, 103, LINE, 2.5, both=True)
    s.line(382, 169, 411, 169, LINE, 2.5, both=True)
    # AHB
    s.rect(415, 215, 170, 60, "#ffffff", LINE, 8, 1.5)
    s.text(500, 238, "AHB1 (100 MHz)", 13, weight="bold")
    s.text(500, 258, "GPIO A~E, H · RCC · CRC", 11, color=MUTED)
    s.line(382, 245, 411, 245, LINE, 2.5, both=True)
    s.rect(415, 290, 170, 50, "#ffffff", LINE, 8, 1.5)
    s.text(500, 312, "AHB2", 13, weight="bold")
    s.text(500, 330, "USB OTG FS", 11, color=MUTED)
    s.line(382, 315, 411, 315, LINE, 2.5, both=True)
    # APB 브리지
    s.line(587, 245, 620, 245, LINE, 2.5)
    s.line(620, 120, 620, 400, LINE, 2.5)
    s.rect(640, 75, 170, 160, ORANGE_BG, ORANGE, 8, 1.5)
    s.text(725, 98, "APB2 (최대 100 MHz)", 13, weight="bold")
    for i, t in enumerate(["TIM1 · 9 · 10 · 11", "USART1 · USART6", "SPI1 · SPI4 · SPI5", "ADC1 · SYSCFG · EXTI"]):
        s.text(725, 124 + i * 26, t, 12)
    s.line(620, 155, 636, 155, LINE, 2.5, arrow=True)
    s.rect(640, 250, 170, 160, ORANGE_BG, ORANGE, 8, 1.5)
    s.text(725, 273, "APB1 (최대 50 MHz)", 13, weight="bold")
    for i, t in enumerate(["TIM2~5", "USART2", "I2C1~3 · SPI2 · 3", "IWDG · WWDG · RTC"]):
        s.text(725, 299 + i * 26, t, 12)
    s.line(620, 330, 636, 330, LINE, 2.5, arrow=True)
    # 클럭/리셋
    s.box(55, 320, 230, 50, "RCC", "클럭 · 리셋 제어", GRAY_BG, LINE, 13)
    s.box(55, 385, 230, 50, "전원 · 저전력", "PWR · 레귤레이터", GRAY_BG, LINE, 13)
    s.text(500, 375, "모든 주변장치는", 12, color=MUTED)
    s.text(500, 393, "RCC에서 클럭을 켜야 동작한다", 12, color=RED, weight="bold")
    s.save("ch03/fig3-2-stm32f411-block.svg")


def fig_3_3():
    s = Svg(860, 400)
    s.text(430, 30, "그림 3-3  AVR의 하버드 구조와 Cortex-M의 단일 메모리 맵", 16, weight="bold")
    # AVR
    s.text(210, 64, "(가) AVR: 주소 공간이 둘로 나뉜다", 14, weight="bold", color=GREEN)
    s.box(150, 90, 120, 60, "CPU", None, BLUE_BG, BLUE)
    s.rect(50, 200, 140, 150, GREEN_BG, GREEN, 8, 1.5)
    s.text(120, 226, "프로그램 공간", 13, weight="bold")
    s.text(120, 250, "Flash", 13)
    s.text(120, 276, "0x0000 ~ 0x7FFF", 11, color=MUTED)
    s.text(120, 294, "(바이트 주소)", 11, color=MUTED)
    s.rect(230, 200, 140, 150, ORANGE_BG, ORANGE, 8, 1.5)
    s.text(300, 226, "데이터 공간", 13, weight="bold")
    s.text(300, 250, "레지스터 · I/O · SRAM", 12)
    s.text(300, 276, "0x0000 ~ 0x08FF", 11, color=MUTED)
    s.line(185, 152, 130, 196, GREEN, 2.5, arrow=True)
    s.line(235, 152, 290, 196, ORANGE, 2.5, both=True)
    s.text(118, 178, "명령 버스", 11, color=GREEN)
    s.text(305, 178, "데이터 버스", 11, color=ORANGE)
    s.text(210, 378, "같은 숫자 0x0100도 어느 공간이냐에 따라 다른 곳", 12, color=RED, weight="bold")
    # Cortex-M
    s.text(640, 64, "(나) Cortex-M: 4 GB 주소 공간 하나", 14, weight="bold", color=BLUE)
    s.box(580, 90, 120, 60, "CPU", None, BLUE_BG, BLUE)
    s.rect(520, 200, 240, 150, "#ffffff", LINE, 8, 1.5)
    rows = [("0x0800 0000", "Flash", GREEN_BG), ("0x2000 0000", "SRAM", ORANGE_BG), ("0x4000 0000", "주변장치", GRAY_BG)]
    for i, (addr, name, fill) in enumerate(rows):
        s.rect(530, 210 + i * 44, 220, 36, fill, LINE, 4, 1)
        s.text(545, 233 + i * 44, addr, 12, "start", MUTED)
        s.text(735, 233 + i * 44, name, 13, "end", weight="bold")
    s.line(640, 152, 640, 196, BLUE, 2.5, both=True)
    s.text(650, 178, "I-Code · D-Code · 시스템 버스", 11, "start", BLUE)
    s.text(640, 378, "주소 하나가 곧 장소 하나. 포인터 하나로 어디든 읽는다", 12, color=BLUE, weight="bold")
    s.save("ch03/fig3-3-harvard-vs-unified.svg")


def fig_3_4():
    s = Svg(820, 520)
    s.text(410, 30, "그림 3-4  Cortex-M 메모리 맵과 STM32F411의 실제 배치", 16, weight="bold")
    regions = [
        ("0xE000 0000", "시스템 (코어 주변장치)", "NVIC · SysTick · SCB · 디버그", GRAY_BG),
        ("0xA000 0000", "외부 장치", "F411에는 사용하지 않음", "#ffffff"),
        ("0x6000 0000", "외부 RAM", "F411에는 사용하지 않음", "#ffffff"),
        ("0x4000 0000", "주변장치", "APB1 · APB2 · AHB1 · AHB2", ORANGE_BG),
        ("0x2000 0000", "SRAM", "128 KB (0x2000 0000 ~ 0x2001 FFFF)", BLUE_BG),
        ("0x0000 0000", "코드", "Flash 512 KB는 0x0800 0000부터", GREEN_BG),
    ]
    x, w, top, h = 220, 260, 60, 66
    for i, (addr, name, sub, fill) in enumerate(regions):
        y = top + i * h
        s.rect(x, y, w, h, fill, LINE, 0, 1.5)
        s.text(x + w / 2, y + h / 2 - 2, name, 14, weight="bold")
        s.text(x + w / 2, y + h / 2 + 17, sub, 11, color=MUTED)
        s.text(x - 12, y + h - 4, addr, 12, "end", MUTED)
    s.text(x - 12, top + 4, "0xFFFF FFFF", 12, "end", MUTED)
    # 오른쪽 확대: 주변장치
    zx = 540
    s.text(zx + 120, 76, "주변장치 영역 확대", 13, weight="bold", color=ORANGE)
    zoom = [("0x5000 0000", "AHB2", "USB OTG FS"), ("0x4002 3800", "RCC", "AHB1"),
            ("0x4002 0000", "GPIOA", "AHB1 시작"), ("0x4001 0000", "APB2", "TIM1, USART1 ..."),
            ("0x4000 0000", "APB1", "TIM2, USART2, I2C1 ...")]
    for i, (addr, name, sub) in enumerate(zoom):
        y = 90 + i * 46
        s.rect(zx, y, 240, 40, "#ffffff", ORANGE, 4, 1.2)
        s.text(zx + 12, y + 25, addr, 12, "start", MUTED)
        s.text(zx + 130, y + 18, name, 13, "start", weight="bold")
        s.text(zx + 130, y + 33, sub, 10, "start", MUTED)
    s.path(f"M{x + w},{top + 3 * h + h} L{zx - 10},{90 + 5 * 46 - 6}", ORANGE, 1.2)
    s.path(f"M{x + w},{top + 3 * h} L{zx - 10},{90}", ORANGE, 1.2)
    s.text(zx + 120, 350, "1장의 0x4002 0014(GPIOA_ODR)는", 12, color=MUTED)
    s.text(zx + 120, 368, "GPIOA 시작 주소 + 0x14", 12, color=MUTED)
    s.text(410, 480, "주소는 아래가 작고 위가 크다. 칸 크기는 실제 비율과 다르다", 12, color=MUTED)
    s.text(410, 500, "출처: STM32F411 레퍼런스 매뉴얼(RM0383) 메모리 맵 절, Cortex-M4 프로그래밍 매뉴얼(PM0214)", 11, color=MUTED)
    s.save("ch03/fig3-4-memory-map.svg")


def fig_3_5():
    s = Svg(820, 520)
    s.text(410, 30, "그림 3-5  Cortex-M4 코어 레지스터", 16, weight="bold")
    x, w, h = 120, 180, 22
    for i in range(13):
        y = 60 + i * h
        s.rect(x, y, w, h, "#ffffff", LINE, 0, 1)
        s.text(x + w / 2, y + 16, f"R{i}", 13, weight="bold")
    s.path(f"M{x - 10},{62} L{x - 18},{62} L{x - 18},{60 + 13 * h - 2} L{x - 10},{60 + 13 * h - 2}", BLUE, 1.5)
    s.text(x - 26, 60 + 6.5 * h - 8, "범용", 12, "end", BLUE, "bold")
    s.text(x - 26, 60 + 6.5 * h + 10, "레지스터", 12, "end", BLUE, "bold")
    special = [("R13 (SP)", "스택 포인터: MSP / PSP", ORANGE_BG), ("R14 (LR)", "링크 레지스터: 복귀 주소", ORANGE_BG),
               ("R15 (PC)", "프로그램 카운터", ORANGE_BG)]
    for i, (n, d, fill) in enumerate(special):
        y = 60 + (13 + i) * h
        s.rect(x, y, w, h, fill, LINE, 0, 1)
        s.text(x + w / 2, y + 16, n, 13, weight="bold")
        s.text(x + w + 12, y + 16, d, 12, "start", MUTED)
    y = 60 + 16 * h + 12
    s.rect(x, y, w, h, GREEN_BG, LINE, 0, 1)
    s.text(x + w / 2, y + 16, "xPSR", 13, weight="bold")
    s.text(x + w + 12, y + 16, "상태: N Z C V 플래그, 예외 번호", 12, "start", MUTED)
    # 특수 레지스터
    sx = 520
    s.text(sx + 110, 72, "특수 레지스터", 14, weight="bold", color=RED)
    sp = [("PRIMASK", "모든 인터럽트 막기"), ("FAULTMASK", "폴트까지 막기"), ("BASEPRI", "우선순위 이하 막기"),
          ("CONTROL", "스택 선택, 권한")]
    for i, (n, d) in enumerate(sp):
        yy = 86 + i * 50
        s.rect(sx, yy, 220, 40, "#ffffff", RED, 6, 1.2)
        s.text(sx + 12, yy + 18, n, 13, "start", weight="bold")
        s.text(sx + 12, yy + 34, d, 11, "start", MUTED)
    s.text(sx + 110, 310, "FPU 레지스터 (M4F)", 14, weight="bold", color=BLUE)
    s.rect(sx, 324, 220, 40, BLUE_BG, BLUE, 6, 1.2)
    s.text(sx + 110, 349, "S0~S31 · FPSCR", 13, weight="bold")
    s.text(410, 505, "모든 레지스터는 32비트. R0~R3은 함수 인자와 반환값을 전달한다 (AAPCS)", 12, color=MUTED)
    s.save("ch03/fig3-5-core-registers.svg")


def fig_3_6():
    s = Svg(860, 380)
    s.text(430, 30, "그림 3-6  STM32F411의 버스 구조", 16, weight="bold")
    masters = [("Cortex-M4 I-Code", BLUE_BG, BLUE), ("Cortex-M4 D-Code", BLUE_BG, BLUE),
               ("Cortex-M4 시스템", BLUE_BG, BLUE), ("DMA1 · DMA2", ORANGE_BG, ORANGE)]
    for i, (n, fill, st) in enumerate(masters):
        y = 70 + i * 66
        s.box(40, y, 170, 48, n, None, fill, st, 13)
        s.line(212, y + 24, 286, y + 24, LINE, 2, arrow=True)
    s.text(125, 60, "마스터 (요청하는 쪽)", 12, color=MUTED)
    s.rect(290, 60, 120, 260, GRAY_BG, LINE, 10, 2)
    s.text(350, 180, "버스", 15, weight="bold")
    s.text(350, 202, "매트릭스", 15, weight="bold")
    s.text(350, 228, "(AHB)", 12, color=MUTED)
    slaves = [("Flash", "명령 · 상수", GREEN_BG, GREEN), ("SRAM", "변수 · 스택", GREEN_BG, GREEN),
              ("AHB1 주변장치", "GPIO · RCC", "#ffffff", LINE), ("AHB/APB 브리지", "", "#ffffff", LINE)]
    for i, (n, sub, fill, st) in enumerate(slaves):
        y = 70 + i * 66
        s.line(412, y + 24, 486, y + 24, LINE, 2, arrow=True)
        s.box(490, y, 160, 48, n, sub or None, fill, st, 13)
    s.text(570, 60, "슬레이브 (응답하는 쪽)", 12, color=MUTED)
    s.line(652, 70 + 3 * 66 + 24, 690, 70 + 3 * 66 + 24, LINE, 2)
    s.line(690, 150, 690, 330, LINE, 2)
    s.box(705, 125, 130, 50, "APB2", "≤ 100 MHz", ORANGE_BG, ORANGE, 13)
    s.box(705, 295, 130, 50, "APB1", "≤ 50 MHz", ORANGE_BG, ORANGE, 13)
    s.line(690, 150, 701, 150, LINE, 2, arrow=True)
    s.line(690, 320, 701, 320, LINE, 2, arrow=True)
    s.text(430, 355, "마스터가 서로 다른 슬레이브에 접근하면 동시에 진행된다. 같은 슬레이브를 두고는 중재가 일어난다", 12, color=MUTED)
    s.save("ch03/fig3-6-bus.svg")


def fig_3_7():
    s = Svg(880, 400)
    s.text(440, 30, "그림 3-7  STM32F411의 클럭 트리 (단순화, HSI로 100 MHz 만들기)", 16, weight="bold")
    # 소스
    s.box(30, 70, 140, 52, "HSI", "내부 RC 16 MHz", GREEN_BG, GREEN, 14)
    s.box(30, 140, 140, 52, "HSE", "외부 4~26 MHz", GREEN_BG, GREEN, 14)
    s.box(30, 290, 140, 52, "LSI / LSE", "32 kHz", GREEN_BG, GREEN, 14)
    s.line(172, 96, 210, 96, LINE, 2)
    s.line(172, 166, 210, 166, LINE, 2)
    s.line(210, 96, 210, 166, LINE, 2)
    s.line(210, 131, 236, 131, LINE, 2, arrow=True)
    # PLL
    s.rect(240, 70, 300, 125, BLUE_BG, BLUE, 10, 2)
    s.text(390, 92, "PLL", 15, weight="bold")
    s.box(252, 105, 76, 48, "÷M", "M = 8", "#ffffff", BLUE, 13)
    s.box(352, 105, 76, 48, "×N", "N = 100", "#ffffff", BLUE, 13)
    s.box(452, 105, 76, 48, "÷P", "P = 2", "#ffffff", BLUE, 13)
    s.line(330, 129, 349, 129, LINE, 2, arrow=True)
    s.line(430, 129, 449, 129, LINE, 2, arrow=True)
    s.text(340, 172, "2 MHz", 11, color=BLUE)
    s.text(440, 172, "200 MHz (VCO)", 11, color=BLUE)
    s.text(290, 186, "", 1)
    # SYSCLK mux
    s.poly([(580, 70), (610, 85), (610, 180), (580, 195)], GRAY_BG)
    s.text(595, 137, "SW", 11, weight="bold")
    s.line(530, 129, 578, 129, LINE, 2, arrow=True)
    s.path("M172,96 L200,96 L200,58 L565,58 L565,90 L578,90", MUTED, 1.5, arrow=True)
    s.text(380, 52, "HSI 직접 (리셋 직후 기본값)", 11, color=MUTED)
    s.line(612, 132, 650, 132, LINE, 2.5, arrow=True)
    s.box(652, 108, 100, 48, "SYSCLK", "100 MHz", ORANGE_BG, ORANGE, 13)
    # AHB, APB
    s.line(702, 158, 702, 210, LINE, 2, arrow=True)
    s.box(652, 212, 100, 44, "AHB ÷1", "HCLK 100", ORANGE_BG, ORANGE, 12)
    s.line(702, 258, 702, 280, LINE, 2)
    s.line(600, 280, 820, 280, LINE, 2)
    s.line(600, 280, 600, 300, LINE, 2, arrow=True)
    s.line(820, 280, 820, 300, LINE, 2, arrow=True)
    s.box(540, 302, 120, 44, "APB1 ÷2", "50 MHz", "#ffffff", ORANGE, 12)
    s.box(760, 302, 110, 44, "APB2 ÷1", "100 MHz", "#ffffff", ORANGE, 12)
    s.line(754, 234, 790, 234, LINE, 2, arrow=True)
    s.text(796, 230, "코어 · 메모리", 11, "start", MUTED)
    s.text(796, 245, "AHB 주변장치", 11, "start", MUTED)
    s.line(172, 316, 230, 316, LINE, 2, arrow=True)
    s.text(236, 312, "RTC · 독립 워치독", 12, "start", MUTED)
    s.text(440, 380, "VCO 입력은 1~2 MHz, VCO 출력은 100~432 MHz 범위 안에서 고른다 (RM0383 RCC 절, DS10314)", 11, color=MUTED)
    s.save("ch03/fig3-7-clock-tree.svg")


# ---------------------------------------------------------------------------
# 4장
# ---------------------------------------------------------------------------

def bit_row(s, x, y, bits, hl=None, fill=None, size=24, label=None, hl_color=ORANGE):
    """8비트 값을 칸으로 그린다. bits는 '10100110' 같은 문자열(왼쪽이 7번 비트)."""
    for i, b in enumerate(bits):
        n = 7 - i
        on = hl is not None and n == hl
        f = (ORANGE_BG if hl_color == ORANGE else BLUE_BG) if on else (fill or "#ffffff")
        s.rect(x + i * size, y, size, size, f, hl_color if on else MUTED, 2, 1.5 if on else 1)
        s.text(x + i * size + size / 2, y + size / 2 + 5, b, 13, weight="bold" if on else "normal")
    if label:
        s.text(x - 10, y + size / 2 + 5, label, 12, "end", INK)


def mono(s, x, y, txt, size=13, color=INK, anchor="start", weight="normal"):
    s.add(
        f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" text-anchor="{anchor}" fill="{color}" '
        f'font-weight="{weight}" xml:space="preserve" font-family="Consolas,\'D2Coding\',\'Noto Sans Mono\',monospace">{escape(txt)}</text>'
    )


def fig_4_1():
    s = Svg(860, 470)
    s.text(430, 30, "그림 4-1  비트 설정, 해제, 반전, 검사", 16, weight="bold")
    x0 = "10100110"
    panels = [
        ("설정 (set)", "x |= (1 << 3)", "OR: 1인 자리만 1로", "00001000", "10101110", 3, GREEN),
        ("해제 (clear)", "x &= ~(1 << 2)", "AND: 0인 자리만 0으로", "11111011", "10100010", 2, RED),
        ("반전 (toggle)", "x ^= (1 << 7)", "XOR: 1인 자리만 뒤집기", "10000000", "00100110", 7, BLUE),
        ("검사 (test)", "x & (1 << 5)", "AND 결과가 0이 아니면 참", "00100000", "00100000", 5, ORANGE),
    ]
    for k, (title, code, desc, mask, res, n, col) in enumerate(panels):
        px = 40 + (k % 2) * 410
        py = 60 + (k // 2) * 200
        s.rect(px, py, 380, 180, "#fafbfc", col, 10, 1.5)
        s.text(px + 16, py + 26, title, 14, "start", col, "bold")
        mono(s, px + 364, py + 26, code, 13, INK, "end", "bold")
        bx = px + 150
        bit_row(s, bx, py + 44, x0, n, label="x = 0xA6")
        bit_row(s, bx, py + 78, mask, n, label="마스크")
        s.line(bx, py + 110, bx + 8 * 24, py + 110, LINE, 1.5)
        bit_row(s, bx, py + 116, res, n, label="결과")
        s.text(px + 190, py + 166, desc, 12, color=MUTED)
        for i in range(8):
            s.text(bx + i * 24 + 12, py + 40, str(7 - i), 9, color=MUTED)
    s.text(430, 460, "주황색 칸이 바뀌거나 검사하는 비트. 나머지 비트는 그대로 남는다", 12, color=MUTED)
    s.save("ch04/fig4-1-bit-ops.svg")


def fig_4_2():
    s = Svg(900, 440)
    s.text(450, 30, "그림 4-2  레지스터 주소와 구조체 멤버가 겹쳐지는 모습", 16, weight="bold")
    regs = ["MODER", "OTYPER", "OSPEEDR", "PUPDR", "IDR", "ODR", "BSRR", "LCKR"]
    x, y0, h = 140, 70, 34
    s.text(x + 90, 60, "메모리 (GPIOA 영역)", 13, weight="bold", color=ORANGE)
    s.text(620, 60, "C 구조체 GPIO_TypeDef", 13, weight="bold", color=BLUE)
    for i, r in enumerate(regs):
        y = y0 + i * h
        on = r == "ODR"
        s.rect(x, y, 180, h, ORANGE_BG if on else "#ffffff", ORANGE if on else LINE, 0, 1.5 if on else 1)
        s.text(x + 90, y + 22, "32비트", 11, color=MUTED)
        mono(s, x - 10, y + 22, f"0x4002 00{i * 4:02X}", 12, MUTED, "end")
        # 구조체 줄
        mono(s, 470, y + 22, f"volatile uint32_t {r + ';':<9}", 13, INK if not on else BLUE, "start", "bold" if on else "normal")
        mono(s, 870, y + 22, f"/* 0x{i * 4:02X} */", 12, MUTED, "end")
        s.line(322, y + h / 2, 462, y + h / 2, BLUE if on else "#c9d2dc", 2 if on else 1, "4 3" if not on else None, arrow=on)
    s.rect(455, y0 - 4, 425, len(regs) * h + 8, "none", BLUE, 8, 1.5)
    mono(s, 140, y0 + 8 * h + 40, "GPIOA = (GPIO_TypeDef *)0x40020000", 13, INK, "start", "bold")
    mono(s, 140, y0 + 8 * h + 64, "GPIOA->ODR  →  0x40020000 + 0x14 = 0x40020014", 13, BLUE, "start", "bold")
    s.text(450, 425, "구조체의 시작을 주변장치의 시작 주소에 맞추면, 멤버 하나하나가 레지스터 하나하나에 겹쳐진다", 12, color=MUTED)
    s.save("ch04/fig4-2-struct-overlay.svg")


def fig_4_3():
    s = Svg(880, 430)
    s.text(440, 30, "그림 4-3  volatile이 없을 때 컴파일러가 루프를 바꿔 버리는 과정", 16, weight="bold")

    def code_box(x, y, w, lines, title, col, fill):
        s.rect(x, y, w, 30 + len(lines) * 22, fill, col, 8, 1.5)
        s.text(x + w / 2, y + 20, title, 13, weight="bold", color=col)
        for i, (t, c) in enumerate(lines):
            mono(s, x + 14, y + 46 + i * 22, t, 13, c or INK)

    code_box(30, 60, 260, [("uint8_t ready = 0;", None), ("", None), ("void ISR(void) {", None),
                           ("    ready = 1;", BLUE), ("}", None), ("", None), ("int main(void) {", None),
                           ("    while (ready == 0) {", RED), ("    }", RED), ("    start();", None), ("}", None)],
             "C 코드", LINE, "#ffffff")
    code_box(330, 60, 250, [("    ldrb r3, [ready]", ORANGE), ("loop:", None), ("    cmp  r3, #0", None),
                            ("    beq  loop", RED), ("    bl   start", None)],
             "volatile 없음 (최적화)", RED, "#fdecec")
    s.text(455, 230, "ready를 한 번만 읽고", 12, color=RED)
    s.text(455, 248, "레지스터 r3 값만 계속 비교", 12, color=RED)
    s.text(455, 266, "→ ISR이 1로 바꿔도 영원히 루프", 12, color=RED, weight="bold")
    code_box(610, 60, 250, [("loop:", None), ("    ldrb r3, [ready]", GREEN), ("    cmp  r3, #0", None),
                            ("    beq  loop", None), ("    bl   start", None)],
             "volatile 있음", GREEN, GREEN_BG)
    s.text(735, 230, "매번 메모리에서 다시 읽는다", 12, color=GREEN)
    s.text(735, 248, "→ ISR이 바꾼 값을 본다", 12, color=GREEN, weight="bold")
    s.rect(330, 300, 530, 56, "#ffffff", MUTED, 8, 1)
    s.text(595, 322, "컴파일러의 생각: \"루프 안에서 ready를 바꾸는 코드가 없으니 값은 변하지 않는다\"", 12, weight="bold")
    s.text(595, 342, "ISR은 컴파일러가 보기에 main과 상관없이 따로 불리는 함수라서, 그 효과를 계산에 넣지 않는다", 11, color=MUTED)
    s.text(440, 410, "어셈블리는 Cortex-M(Thumb-2)을 단순화한 것이다. 실제 출력은 컴파일러와 옵션에 따라 다르다", 12, color=MUTED)
    s.save("ch04/fig4-3-volatile.svg")


# ---------------------------------------------------------------------------
# 5장
# ---------------------------------------------------------------------------

def chain(s, x, y, items, w=128, h=50, gap=30, colors=None):
    """상자들을 가로로 잇는다. items = [(제목, 부제), ...]"""
    for i, (a, b) in enumerate(items):
        fill, st = (colors[i] if colors else (GRAY_BG, LINE))
        s.box(x + i * (w + gap), y, w, h, a, b, fill, st, 13)
        if i < len(items) - 1:
            s.line(x + i * (w + gap) + w + 2, y + h / 2, x + (i + 1) * (w + gap) - 3, y + h / 2, LINE, 2, arrow=True)


def fig_5_1():
    s = Svg(900, 450)
    s.text(450, 30, "그림 5-1  보드별 업로드 경로 비교", 16, weight="bold")
    PC = (GREEN_BG, GREEN)
    MID = (GRAY_BG, LINE)
    TGT = (BLUE_BG, BLUE)
    rows = [
        ("아두이노 우노", [("PC", "avrdude"), ("ATmega16U2", "USB ↔ 시리얼"), ("UART", "TX · RX · DTR"),
                         ("부트로더", "Optiboot"), ("Flash", "ATmega328P")], "USB 케이블 하나. 리셋 직후 부트로더가 받아 쓴다"),
        ("ATmega128", [("PC", "avrdude"), ("USBasp", "ISP 프로그래머"), ("ISP", "SCK · PDI · PDO · RST"),
                       ("프로그래밍 회로", "칩 내장"), ("Flash", "ATmega128A")], "부트로더 없이 칩에 직접 쓴다. 퓨즈도 이 경로로만 바꾼다"),
        ("STM32", [("PC", "OpenOCD · GDB"), ("ST-LINK", "보드 내장"), ("SWD", "SWDIO · SWCLK"),
                   ("디버그 포트", "코어 내장"), ("Flash", "STM32F411")], "업로드와 디버깅(정지, 한 줄 실행)을 같은 선으로 한다"),
        ("ESP32", [("PC", "esptool"), ("USB-UART", "CP2102 · CH340"), ("UART0", "TX · RX · EN · IO0"),
                   ("ROM 부트로더", "칩 내장"), ("SPI Flash", "외부 4 MB")], "IO0을 Low로 두고 리셋하면 다운로드 모드로 들어간다"),
    ]
    for r, (name, items, note) in enumerate(rows):
        y = 60 + r * 96
        s.text(20, y + 30, name, 14, "start", weight="bold")
        chain(s, 140, y, items, 128, 50, 25, [PC, MID, MID, TGT, TGT])
        s.text(140, y + 70, note, 12, "start", MUTED)
    s.save("ch05/fig5-1-upload-paths.svg")


def fig_5_2():
    s = Svg(900, 330)
    s.text(450, 30, "그림 5-2  아두이노 IDE가 [업로드] 버튼 뒤에서 하는 일", 16, weight="bold")
    steps = [("스케치", ".ino"), ("전처리", ".cpp로 변환"), ("컴파일", "avr-g++"), ("링크", "코어 라이브러리"),
             ("변환", "avr-objcopy → .hex"), ("업로드", "avrdude")]
    for i, (a, b) in enumerate(steps):
        x = 20 + i * 147
        fill, st = (GREEN_BG, GREEN) if i == 0 else ((BLUE_BG, BLUE) if i < 5 else (ORANGE_BG, ORANGE))
        s.box(x, 70, 127, 60, a, b, fill, st, 14)
        if i < 5:
            s.line(x + 129, 100, x + 145, 100, LINE, 2, arrow=True)
    notes = [
        (83, ["#include <Arduino.h>", "추가", "함수 원형 자동 생성"]),
        (377, ["digitalWrite, Serial 등", "아두이노 코어와", "main() 함수를 함께 링크"]),
        (818, ["보드를 리셋하고", "부트로더에", "115200 bps로 전송"]),
    ]
    for cx, lines in notes:
        for j, t in enumerate(lines):
            s.text(cx + (147 if cx == 83 else 0), 160 + j * 18, t, 12, color=MUTED)
    s.rect(60, 240, 780, 66, "#fafbfc", MUTED, 8, 1)
    s.text(450, 264, "환경 설정에서 \"컴파일과 업로드 중 자세한 출력 표시\"를 켜면 실제로 실행되는 명령이 모두 보인다", 13, weight="bold")
    s.text(450, 288, "이 장에서는 같은 일을 avr-gcc, avr-objcopy, avrdude를 직접 실행해서 해 본다", 12, color=MUTED)
    s.save("ch05/fig5-2-arduino-build.svg")


def fig_5_3():
    s = Svg(880, 430)
    s.text(440, 30, "그림 5-3  USBasp와 ATmega128의 ISP 연결", 16, weight="bold")
    # 10핀 헤더
    s.text(135, 66, "USBasp 10핀 ISP 커넥터", 14, weight="bold", color=BLUE)
    s.text(135, 84, "(위에서 본 모습, ▶가 1번 핀)", 11, color=MUTED)
    pins = {1: "MOSI", 2: "VCC", 3: "NC", 4: "GND", 5: "RST", 6: "GND", 7: "SCK", 8: "GND", 9: "MISO", 10: "GND"}
    x0, y0 = 65, 100
    for n in range(1, 11):
        col = (n - 1) % 2
        row = (n - 1) // 2
        x = x0 + col * 70
        y = y0 + row * 52
        name = pins[n]
        used = name in ("MOSI", "MISO", "SCK", "RST", "GND")
        s.rect(x, y, 60, 40, BLUE_BG if used else "#ffffff", BLUE if used else MUTED, 4, 1.2)
        s.text(x + 30, y + 17, str(n), 11, color=MUTED)
        s.text(x + 30, y + 33, name, 12, weight="bold")
    s.poly([(x0 - 14, y0 + 14), (x0 - 4, y0 + 20), (x0 - 14, y0 + 26)], BLUE, BLUE, 1)
    # ATmega128 핀
    s.rect(520, 80, 300, 270, "#fafbfc", ORANGE, 12, 1.5)
    s.text(670, 104, "ATmega128A", 15, weight="bold", color=ORANGE)
    targets = [("PE0 (PDI)", "1번 MOSI", "데이터 입력"), ("PE1 (PDO)", "9번 MISO", "데이터 출력"),
               ("PB1 (SCK)", "7번 SCK", "클럭"), ("RESET", "5번 RST", "리셋"), ("GND", "4·6·8·10번", "공통 접지")]
    s.text(400, 104, "USBasp 핀", 12, weight="bold", color=BLUE)
    for i, (pin, src, desc) in enumerate(targets):
        y = 125 + i * 44
        s.rect(340, y, 120, 34, BLUE_BG, BLUE, 4, 1.2)
        s.text(400, y + 22, src, 12, weight="bold")
        s.line(462, y + 17, 536, y + 17, ORANGE if i < 3 else LINE, 2, arrow=True)
        s.rect(540, y, 120, 34, ORANGE_BG, ORANGE, 4, 1.2)
        s.text(600, y + 22, pin, 13, weight="bold")
        s.text(672, y + 22, desc, 11, "start", MUTED)
    s.rect(60, 370, 760, 46, ORANGE_BG, ORANGE, 8, 1.2)
    s.text(440, 391, "주의: ATmega128의 ISP는 SPI 핀(PB2 MOSI, PB3 MISO)이 아니라 PE0, PE1을 쓴다", 13, weight="bold", color=RED)
    s.text(440, 408, "키트의 ISP 커넥터가 이미 올바르게 배선되어 있는지 회로도로 확인한다", 12, color=MUTED)
    s.save("ch05/fig5-3-usbasp-isp.svg")


def fig_5_4():
    s = Svg(900, 400)
    s.text(450, 30, "그림 5-4  툴체인의 구성", 16, weight="bold")
    items = [("소스", ".c · .h", GREEN_BG, GREEN), ("컴파일러", "gcc → .o", BLUE_BG, BLUE),
             ("링커", "ld + 링커 스크립트", BLUE_BG, BLUE), ("실행 파일", ".elf", "#ffffff", LINE),
             ("objcopy", "→ .bin · .hex", BLUE_BG, BLUE)]
    for i, (a, b, f, st) in enumerate(items):
        x = 30 + i * 172
        s.box(x, 70, 150, 60, a, b, f, st, 14)
        if i < 4:
            s.line(x + 152, 100, x + 170, 100, LINE, 2, arrow=True)
    s.text(450, 58, "빌드: PC 안에서 일어나는 일", 12, color=MUTED)
    # 아래: 업로드와 디버깅
    s.box(130, 200, 150, 60, "GDB", "디버거", ORANGE_BG, ORANGE, 14)
    s.box(370, 200, 150, 60, "OpenOCD", "GDB 서버", ORANGE_BG, ORANGE, 14)
    s.box(610, 200, 120, 60, "ST-LINK", "USB ↔ SWD", GRAY_BG, LINE, 14)
    s.box(770, 200, 110, 60, "STM32", "타깃", BLUE_BG, BLUE, 14)
    s.line(282, 230, 366, 230, LINE, 2, both=True)
    s.text(324, 222, "TCP 3333", 11, color=MUTED)
    s.line(522, 230, 606, 230, LINE, 2, both=True)
    s.text(564, 222, "USB", 11, color=MUTED)
    s.line(732, 230, 766, 230, LINE, 2, both=True)
    s.path("M535,132 L535,165 L205,165 L205,196", MUTED, 1.5, arrow=True)
    s.text(370, 160, ".elf (심볼 포함)", 11, color=MUTED)
    s.path("M800,132 L800,180 L445,180 L445,196", MUTED, 1.5, arrow=True)
    s.text(620, 176, ".elf 또는 .bin 업로드", 11, color=MUTED)
    rows = [("arm-none-eabi-gcc", "STM32용 크로스 컴파일러, 링커, objcopy 묶음"),
            ("avr-gcc · avr-libc", "AVR용 크로스 컴파일러와 표준 라이브러리"),
            ("avrdude", "AVR 업로드 도구 (부트로더, USBasp 모두 지원)"),
            ("OpenOCD", "디버그 프로브를 GDB와 이어 주는 서버")]
    for i, (a, b) in enumerate(rows):
        s.text(130, 300 + i * 22, a, 12, "start", weight="bold")
        s.text(300, 300 + i * 22, b, 12, "start", MUTED)
    s.save("ch05/fig5-4-toolchain.svg")


def fig_5_5():
    s = Svg(900, 470)
    s.text(450, 30, "그림 5-5  VS Code 디버그 화면의 구성 (개념도)", 16, weight="bold")
    s.rect(30, 50, 840, 400, "#ffffff", LINE, 8, 1.5)
    s.rect(30, 50, 840, 34, GRAY_BG, LINE, 8, 1)
    # 디버그 툴바
    s.text(450, 72, "① 디버그 툴바:  계속 · 한 줄 실행 · 함수 안으로 · 함수 밖으로 · 다시 시작 · 중지", 12, weight="bold")
    # 왼쪽 패널
    s.rect(40, 94, 250, 346, "#fafbfc", MUTED, 6, 1)
    panels = [("② 변수", ["count = 42", "ready = 1 '\\001'"]), ("③ 조사식", ["GPIOA->ODR = 0x20"]),
              ("④ 호출 스택", ["led_toggle()  led.c:18", "main()        main.c:29"]),
              ("⑤ 주변장치 레지스터", ["GPIOA ▸ MODER, ODR ...", "RCC ▸ AHB1ENR ..."])]
    y = 104
    for title, lines in panels:
        s.text(52, y + 14, title, 12, "start", BLUE, "bold")
        for j, t in enumerate(lines):
            mono(s, 64, y + 34 + j * 18, t, 11, INK)
        y += 40 + len(lines) * 18 + 16
    # 코드 영역
    s.rect(300, 94, 560, 250, "#ffffff", MUTED, 6, 1)
    code = ["int main(void)", "{", "    led_init();", "    for (;;) {", "        led_toggle();",
            "        delay(1000000);", "    }", "}"]
    for i, t in enumerate(code):
        y = 120 + i * 26
        s.text(345, y, str(25 + i), 11, "end", MUTED)
        if i == 4:
            s.rect(352, y - 17, 496, 24, "#fff4cc", ORANGE, 3, 1)
            s.poly([(316, y - 10), (326, y - 5), (316, y)], ORANGE, ORANGE, 1)
        if i == 5:
            s.add(f'<circle cx="{318:.1f}" cy="{y - 5:.1f}" r="6" fill="{RED}"/>')
        mono(s, 360, y, t, 13)
    s.text(600, 330, "⑥ 노란 줄: 다음에 실행할 줄,  빨간 점: 중단점", 12, color=MUTED)
    # 터미널
    s.rect(300, 352, 560, 88, "#1f2933", LINE, 6, 1)
    mono(s, 315, 376, "⑦ 디버그 콘솔 / 터미널", 12, "#e3edfd", "start", "bold")
    mono(s, 315, 398, "Info : STLINK V2J... (API v2) VID:PID 0483:374B", 11, "#c9d2dc")
    mono(s, 315, 418, "Info : stm32f4x.cpu: Cortex-M4 r0p1 processor detected", 11, "#c9d2dc")
    s.save("ch05/fig5-5-vscode-debug.svg")


# ---------------------------------------------------------------------------
# 6장
# ---------------------------------------------------------------------------

def fig_6_0():
    s = Svg(880, 430)
    s.text(440, 30, "그림 6-0  AVR 포트 레지스터 세 개(DDR, PORT, PIN)의 역할", 16, weight="bold")
    # 레지스터 상자
    s.box(40, 70, 170, 56, "DDRxn", "방향: 0 입력, 1 출력", BLUE_BG, BLUE, 14)
    s.box(40, 150, 170, 56, "PORTxn", "출력 값 / 풀업 켜기", BLUE_BG, BLUE, 14)
    s.box(40, 250, 170, 56, "PINxn", "핀 전압을 읽은 값", GREEN_BG, GREEN, 14)
    # 출력 버퍼
    s.poly([(330, 150), (330, 206), (380, 178)], "#ffffff", LINE, 2)
    s.text(355, 230, "출력 버퍼", 11, color=MUTED)
    s.line(212, 178, 328, 178, BLUE, 2.5, arrow=True)
    s.line(212, 98, 355, 98, BLUE, 2)
    s.line(355, 98, 355, 160, BLUE, 2, arrow=True)
    s.text(285, 90, "버퍼를 켜고 끈다", 11, color=BLUE)
    # 풀업
    supply(s, 470, 70, "VCC")
    resistor_v(s, 470, 70, 140, "내부 풀업")
    s.rect(446, 140, 48, 24, "#ffffff", ORANGE, 4, 1.5)
    s.text(470, 157, "스위치", 10, color=ORANGE)
    s.line(470, 164, 470, 178, LINE, 2)
    s.path("M212,166 L300,166 L300,128 L420,128 L420,152 L443,152", ORANGE, 1.5, arrow=True)
    s.text(502, 157, "← DDR=0, PORT=1일 때만 닫힘", 11, "start", ORANGE)
    # 핀 노드
    s.line(382, 178, 640, 178, LINE, 2.5)
    dot(s, 470, 178)
    dot(s, 560, 178)
    s.rect(640, 160, 60, 36, ORANGE_BG, ORANGE, 4, 2)
    s.text(670, 184, "핀", 14, weight="bold")
    # 입력 경로
    s.line(560, 178, 560, 278, LINE, 2)
    s.poly([(560, 278), (510, 258), (510, 298)], "#ffffff", LINE, 2)
    s.text(535, 320, "슈미트 트리거 + 동기화", 11, color=MUTED)
    s.line(508, 278, 214, 278, GREEN, 2.5, arrow=True)
    # 표
    tx, ty = 610, 240
    hdr = ["DDR", "PORT", "핀 상태"]
    ws = [50, 55, 160]
    rows = [("0", "0", "입력, 떠 있음 (Hi-Z)"), ("0", "1", "입력, 내부 풀업"), ("1", "0", "출력 Low"), ("1", "1", "출력 High")]
    x = tx
    for h, w in zip(hdr, ws):
        s.rect(x, ty, w, 26, GRAY_BG, LINE, 0, 1)
        s.text(x + w / 2, ty + 18, h, 12, weight="bold")
        x += w
    for r, row in enumerate(rows):
        x = tx
        for v, w in zip(row, ws):
            s.rect(x, ty + 26 * (r + 1), w, 26, "#ffffff", LINE, 0, 1)
            s.text(x + w / 2, ty + 26 * (r + 1) + 18, v, 12)
            x += w
    s.text(440, 410, "PORT는 \"내가 쓴 값\", PIN은 \"핀에서 실제로 읽은 값\"이다. 출력 중에도 PIN으로 핀 전압을 읽을 수 있다", 12, color=MUTED)
    s.save("ch06/fig6-0-avr-port-registers.svg")


def fig_6_1():
    s = Svg(900, 470)
    s.text(450, 30, "그림 6-1  GPIO 핀 하나의 내부 구조 (STM32를 단순화)", 16, weight="bold")
    s.rect(30, 50, 700, 400, "#fafbfc", MUTED, 12, 1.2)
    s.text(60, 72, "칩 내부", 12, "start", MUTED)
    # 출력 경로
    s.box(50, 100, 140, 50, "ODR / BSRR", "출력 데이터", BLUE_BG, BLUE, 13)
    s.box(50, 165, 140, 50, "주변장치 출력", "대체 기능 (AF)", ORANGE_BG, ORANGE, 13)
    s.poly([(230, 105), (260, 120), (260, 195), (230, 210)], GRAY_BG)
    s.text(245, 162, "MUX", 10, weight="bold")
    s.line(192, 125, 228, 125, LINE, 2, arrow=True)
    s.line(192, 190, 228, 190, LINE, 2, arrow=True)
    s.text(245, 98, "MODER", 10, color=MUTED)
    s.rect(290, 95, 140, 120, BLUE_BG, BLUE, 8, 1.5)
    s.text(360, 118, "출력 드라이버", 13, weight="bold")
    s.text(360, 142, "P-MOS (VDD 쪽)", 11)
    s.text(360, 160, "N-MOS (GND 쪽)", 11)
    s.text(360, 184, "OTYPER: 푸시풀/오픈드레인", 10, color=MUTED)
    s.text(360, 200, "OSPEEDR: 속도", 10, color=MUTED)
    s.line(262, 157, 286, 157, LINE, 2, arrow=True)
    # 핀 노드
    s.line(432, 157, 600, 157, LINE, 2.5)
    s.line(600, 157, 600, 330, LINE, 2.5)
    dot(s, 600, 250)
    # 풀업/풀다운
    supply(s, 520, 230, "VDD")
    resistor_v(s, 520, 230, 290, None)
    s.text(470, 268, "풀업", 11, color=MUTED)
    s.line(520, 290, 520, 300, LINE, 2)
    s.line(520, 300, 600, 300, LINE, 2)
    s.text(560, 294, "PUPDR", 10, color=ORANGE)
    # 보호 다이오드
    s.rect(620, 200, 90, 100, "#ffffff", RED, 6, 1.2)
    s.text(665, 225, "보호", 12, weight="bold", color=RED)
    s.text(665, 243, "다이오드", 12, weight="bold", color=RED)
    s.text(665, 265, "VDD · GND로", 10, color=MUTED)
    s.text(665, 280, "전류를 흘림", 10, color=MUTED)
    s.line(600, 250, 618, 250, RED, 1.5)
    # 패드
    s.line(600, 330, 760, 330, LINE, 2.5)
    s.rect(760, 310, 80, 40, ORANGE_BG, ORANGE, 4, 2)
    s.text(800, 336, "핀 (패드)", 13, weight="bold")
    # 입력 경로
    s.line(600, 330, 600, 390, LINE, 2)
    s.poly([(560, 375), (560, 405), (530, 390)], "#ffffff", LINE, 2)
    s.line(600, 390, 562, 390, LINE, 2)
    s.text(545, 425, "슈미트 트리거", 11, color=MUTED)
    s.line(528, 390, 194, 390, GREEN, 2.5, arrow=True)
    s.box(50, 365, 140, 50, "IDR", "입력 데이터", GREEN_BG, GREEN, 13)
    s.line(360, 390, 360, 302, ORANGE, 1.5, arrow=True)
    s.box(290, 255, 140, 44, "주변장치 입력", "AF 입력 (RX 등)", ORANGE_BG, ORANGE, 12)
    s.path("M600,350 L680,350 L680,420 L700,420", MUTED, 1.5, arrow=True)
    s.text(640, 440, "아날로그 (ADC)", 11, color=MUTED)
    s.text(450, 462, "출력은 MODER로 고른 경로가 드라이버를 움직이고, 입력은 모드와 상관없이 항상 IDR로 들어온다(아날로그 모드 제외)", 11, color=MUTED)
    s.save("ch06/fig6-1-gpio-internal.svg")


def fig_6_2():
    s = Svg(900, 330)
    s.text(450, 30, "그림 6-2  GPIOx_MODER의 비트 배치와 PA5 설정", 16, weight="bold")
    x0, y0, w = 40, 90, 51
    for pin in range(15, -1, -1):
        i = 15 - pin
        x = x0 + i * w
        on = pin == 5
        s.rect(x, y0, w, 40, ORANGE_BG if on else "#ffffff", ORANGE if on else LINE, 0, 2 if on else 1)
        s.text(x + w / 2, y0 + 17, f"MODER{pin}", 10, weight="bold" if on else "normal")
        s.text(x + w / 2, y0 + 33, "0 1" if on else "· ·", 11, color=INK if on else MUTED, weight="bold" if on else "normal")
        s.text(x + 5, y0 - 6, str(pin * 2 + 1), 9, "start", MUTED)
        s.text(x + w - 5, y0 - 6, str(pin * 2), 9, "end", MUTED)
    s.text(x0, 66, "비트 번호 →", 11, "start", MUTED)
    xm = x0 + 10 * w
    s.path(f"M{xm + w / 2},{y0 + 42} L{xm + w / 2},{y0 + 70}", ORANGE, 1.5, arrow=True)
    s.text(xm + w / 2, y0 + 88, "PA5 = 비트 11:10", 13, weight="bold", color=ORANGE)
    rows = [("00", "입력 (리셋 기본값)"), ("01", "범용 출력"), ("10", "대체 기능 (AF)"), ("11", "아날로그")]
    for i, (v, d) in enumerate(rows):
        x = 60 + i * 205
        s.rect(x, 210, 190, 40, BLUE_BG if v == "01" else "#ffffff", BLUE if v == "01" else LINE, 6, 1.2)
        s.text(x + 20, 236, v, 14, "start", weight="bold")
        s.text(x + 60, 236, d, 12, "start")
    s.add(f'<text x="450" y="290" font-size="13" text-anchor="middle" fill="{INK}" xml:space="preserve" '
          f'font-family="Consolas,\'D2Coding\',\'Noto Sans Mono\',monospace" font-weight="bold">'
          f'GPIOA-&gt;MODER = (GPIOA-&gt;MODER &amp; ~(3u &lt;&lt; 10)) | (1u &lt;&lt; 10);</text>')
    s.text(450, 314, "핀 n의 모드는 비트 2n+1:2n. 두 비트를 먼저 지우고 원하는 값을 쓴다", 12, color=MUTED)
    s.save("ch06/fig6-2-moder.svg")


def fig_6_3():
    s = Svg(900, 440)
    s.text(450, 30, "그림 6-3  ODR 읽기-수정-쓰기와 BSRR 원자적 쓰기", 16, weight="bold")
    # 위: ODR RMW
    s.text(40, 70, "(가) GPIOA->ODR ^= (1u << 5);  →  명령 세 개로 나뉜다", 14, "start", RED, "bold")
    steps = [("LDR", "ODR 읽기", "0x0000"), ("EOR", "5번 비트 반전", "0x0020"), ("STR", "ODR 쓰기", "0x0020")]
    for i, (op, d, v) in enumerate(steps):
        x = 60 + i * 280
        s.box(x, 90, 150, 54, op, d, BLUE_BG, BLUE, 14)
        s.text(x + 75, 162, f"r3 = {v}", 12, color=MUTED)
    s.line(212, 117, 338, 117, LINE, 2, arrow=True)
    s.line(492, 117, 618, 117, LINE, 2, arrow=True)
    # 인터럽트 끼어듦
    s.rect(225, 175, 100, 50, ORANGE_BG, ORANGE, 8, 1.5)
    s.text(275, 197, "인터럽트!", 13, weight="bold", color=ORANGE)
    s.text(275, 214, "ODR 6번 = 1", 11)
    s.path("M240,173 L240,150 L212,150", ORANGE, 1.5, arrow=True)
    s.rect(560, 175, 300, 50, "#fdecec", RED, 8, 1.5)
    s.text(710, 197, "ISR이 켠 6번 비트가 사라진다", 13, weight="bold", color=RED)
    s.text(710, 214, "STR이 읽어 둔 옛 값(0x0000)을 바탕으로 쓰기 때문", 11, color=MUTED)
    # 아래: BSRR
    s.text(40, 280, "(나) GPIOA->BSRR = (1u << 5);  →  쓰기 한 번", 14, "start", GREEN, "bold")
    s.box(60, 300, 150, 54, "STR", "BSRR 쓰기", GREEN_BG, GREEN, 14)
    s.line(212, 327, 300, 327, LINE, 2, arrow=True)
    s.rect(300, 300, 560, 54, "#ffffff", GREEN, 8, 1.5)
    s.text(580, 322, "1을 쓴 비트만 바뀌고, 0을 쓴 비트는 그대로 둔다", 13, weight="bold")
    s.text(580, 342, "읽지 않으므로 끼어들 틈이 없다 (원자적)", 12, color=MUTED)
    # BSRR 배치
    s.rect(60, 375, 400, 34, BLUE_BG, BLUE, 0, 1.2)
    s.text(260, 397, "BR15 … BR0 (비트 31:16) : 1을 쓰면 Low", 12, weight="bold")
    s.rect(460, 375, 400, 34, ORANGE_BG, ORANGE, 0, 1.2)
    s.text(660, 397, "BS15 … BS0 (비트 15:0) : 1을 쓰면 High", 12, weight="bold")
    s.text(450, 430, "같은 핀의 BS와 BR을 동시에 1로 쓰면 BS(High)가 우선한다 (RM0383 GPIO 절)", 11, color=MUTED)
    s.save("ch06/fig6-3-odr-vs-bsrr.svg")


def fig_6_4():
    s = Svg(900, 400)
    s.text(450, 30, "그림 6-4  대체 기능 멀티플렉서: PA2를 USART2_TX로 쓰기", 16, weight="bold")
    afs = [("AF0", "시스템"), ("AF1", "TIM1/2"), ("AF2", "TIM3~5"), ("AF3", "TIM9~11"), ("AF4", "I2C"),
           ("AF5", "SPI"), ("AF6", "SPI"), ("AF7", "USART1/2"), ("AF8", "USART6"), ("…", ""), ("AF15", "이벤트")]
    for i, (a, b) in enumerate(afs):
        y = 56 + i * 28
        on = a == "AF7"
        s.rect(60, y, 170, 24, ORANGE_BG if on else "#ffffff", ORANGE if on else MUTED, 3, 1.5 if on else 1)
        s.text(72, y + 17, a, 12, "start", weight="bold")
        s.text(220, y + 17, b, 11, "end", MUTED if not on else INK)
        s.line(232, y + 12, 330, 120 + i * 10, ORANGE if on else "#c9d2dc", 2 if on else 1)
    s.poly([(330, 100), (380, 130), (380, 260), (330, 290)], GRAY_BG, LINE, 2)
    s.text(355, 200, "MUX", 12, weight="bold")
    s.line(382, 195, 500, 195, ORANGE, 2.5, arrow=True)
    s.box(502, 170, 110, 50, "PA2", "핀", ORANGE_BG, ORANGE, 15)
    s.line(355, 290, 355, 330, BLUE, 1.5)
    s.line(355, 330, 520, 330, BLUE, 1.5)
    s.text(530, 334, "GPIOA_AFRL의 AFRL2 (비트 11:8) = 0111 (7)", 13, "start", BLUE, "bold")
    s.text(530, 356, "그리고 GPIOA_MODER의 MODER2 = 10 (대체 기능)", 13, "start", BLUE, "bold")
    s.text(640, 120, "어떤 핀이 어떤 AF를 가지는지는", 12, "start", MUTED)
    s.text(640, 140, "데이터시트(DS10314)의", 12, "start", MUTED)
    s.text(640, 160, "대체 기능 매핑 표에 있다", 12, "start", MUTED)
    s.text(450, 388, "AF 번호의 의미는 핀마다 다르다. 이 그림의 주변장치 이름은 STM32F411의 일반적인 배정을 단순화한 것이다", 11, color=MUTED)
    s.save("ch06/fig6-4-af-mux.svg")


# ---------------------------------------------------------------------------
# 7장
# ---------------------------------------------------------------------------

def fig_7_1():
    s = Svg(900, 400)
    s.text(450, 30, "그림 7-1  폴링과 인터럽트의 시간 흐름 비교", 16, weight="bold")
    x0, x1 = 150, 860
    ev = 470

    def axis(y, title, sub):
        s.text(20, y + 5, title, 14, "start", weight="bold")
        s.text(20, y + 24, sub, 11, "start", MUTED)
        s.line(x0, y + 30, x1, y + 30, MUTED, 1.2, arrow=True)

    # 폴링
    y = 70
    axis(y, "(가) 폴링", "주기적으로 확인")
    for i in range(7):
        x = x0 + 10 + i * 100
        s.rect(x, y, 70, 22, BLUE_BG, BLUE, 3, 1)
        s.text(x + 35, y + 16, "다른 일", 11)
        s.rect(x + 70, y, 22, 22, ORANGE_BG, ORANGE, 3, 1)
        s.text(x + 81, y + 16, "?", 12, weight="bold")
    s.line(ev, y - 18, ev, y + 40, RED, 2, "4 3")
    s.text(ev, y - 22, "버튼 눌림", 12, color=RED, weight="bold")
    s.path(f"M{ev},{y + 46} L{x0 + 10 + 4 * 100 + 81},{y + 46}", RED, 1.5)
    s.text((ev + x0 + 491) / 2, y + 62, "다음 확인까지 지연", 11, color=RED)
    s.text(x1, y + 62, "확인할 때마다 CPU 시간을 쓴다 (대부분 헛걸음)", 11, "end", MUTED)

    # 인터럽트
    y = 220
    axis(y, "(나) 인터럽트", "일이 생기면 알려 줌")
    s.rect(x0 + 10, y, ev - x0 - 10, 22, BLUE_BG, BLUE, 3, 1)
    s.text((x0 + ev) / 2, y + 16, "메인 코드: 하고 싶은 일", 11)
    s.rect(ev + 10, y - 30, 80, 22, ORANGE_BG, ORANGE, 3, 1.5)
    s.text(ev + 50, y - 14, "ISR", 12, weight="bold")
    s.rect(ev + 100, y, x1 - ev - 110, 22, BLUE_BG, BLUE, 3, 1)
    s.text((ev + 100 + x1) / 2, y + 16, "메인 코드 계속", 11)
    s.path(f"M{ev},{y + 11} L{ev + 10},{y - 19}", ORANGE, 1.5, arrow=True)
    s.path(f"M{ev + 90},{y - 19} L{ev + 100},{y + 11}", ORANGE, 1.5, arrow=True)
    s.line(ev, y - 48, ev, y + 40, RED, 2, "4 3")
    s.text(ev, y - 52, "버튼 눌림", 12, color=RED, weight="bold")
    s.text(ev + 50, y + 62, "곧바로 반응", 11, color=ORANGE, weight="bold")
    s.text(x1, y + 90, "일이 없을 때는 CPU를 다른 일에 쓰거나 잠재울 수 있다 (11장)", 11, "end", MUTED)
    s.save("ch07/fig7-1-polling-vs-interrupt.svg")


def fig_7_2():
    s = Svg(900, 470)
    s.text(450, 30, "그림 7-2  인터럽트 발생부터 복귀까지 (Cortex-M)", 16, weight="bold")
    steps = [("① 요청", "주변장치가 플래그 설정", GREEN_BG, GREEN),
             ("② 스태킹", "레지스터 8개를 스택에", BLUE_BG, BLUE),
             ("③ 벡터 조회", "벡터 테이블에서 ISR 주소", BLUE_BG, BLUE),
             ("④ ISR 실행", "플래그 지우기, 할 일", ORANGE_BG, ORANGE),
             ("⑤ 언스태킹", "레지스터 복원, 복귀", BLUE_BG, BLUE)]
    for i, (a, b, f, st) in enumerate(steps):
        x = 25 + i * 175
        s.box(x, 60, 155, 60, a, b, f, st, 14)
        if i < 4:
            s.line(x + 157, 90, x + 173, 90, LINE, 2, arrow=True)
    s.text(270, 142, "②와 ③은 하드웨어가 동시에 처리 (최소 12클럭)", 12, color=BLUE)
    # 스택 프레임
    s.text(250, 185, "스택에 자동으로 저장되는 것 (예외 스택 프레임)", 14, weight="bold")
    frame = ["xPSR", "PC (돌아갈 주소)", "LR", "R12", "R3", "R2", "R1", "R0"]
    for i, n in enumerate(frame):
        y = 200 + i * 28
        s.rect(150, y, 200, 28, BLUE_BG if i < 2 else "#ffffff", BLUE, 0, 1)
        s.text(250, y + 19, n, 13, weight="bold" if i < 2 else "normal")
        s.text(140, y + 19, f"SP+0x{(7 - i) * 4:02X}", 11, "end", MUTED)
    s.line(360, 200 + 7 * 28 + 14, 400, 200 + 7 * 28 + 14, ORANGE, 2)
    s.text(406, 200 + 7 * 28 + 18, "← 새 SP", 12, "start", ORANGE, "bold")
    s.text(250, 440, "높은 주소 ↑   스택은 아래로 자란다", 11, color=MUTED)
    # 설명
    tx = 500
    notes = [
        ("ISR이 보통 C 함수인 이유", BLUE),
        ("호출 규약(AAPCS)에서 함수가 마음대로 써도 되는", INK),
        ("R0~R3, R12, LR을 하드웨어가 미리 저장해 준다", INK),
        ("", INK),
        ("LR에 들어가는 특별한 값", BLUE),
        ("ISR 진입 시 LR = 0xFFFF FFxx (EXC_RETURN)", INK),
        ("ISR이 이 값으로 복귀하면 하드웨어가 언스태킹", INK),
        ("", INK),
        ("AVR은 훨씬 단순하다", GREEN),
        ("PC만 스택에 넣고(2바이트) 전체 인터럽트(I 비트)를 끈다", INK),
        ("나머지 레지스터는 컴파일러가 ISR 앞뒤에 저장 코드를 넣는다", INK),
    ]
    for i, (t, c) in enumerate(notes):
        if t:
            s.text(tx, 205 + i * 22, t, 13 if c != INK else 12, "start", c, "bold" if c != INK else "normal")
    s.save("ch07/fig7-2-interrupt-sequence.svg")


def fig_7_3():
    s = Svg(900, 480)
    s.text(450, 30, "그림 7-3  벡터 테이블의 구조", 16, weight="bold")
    # AVR
    s.text(210, 64, "(가) ATmega328P: 점프 명령의 목록", 14, weight="bold", color=GREEN)
    avr = [("0x0000", "JMP 리셋 처리"), ("0x0002", "JMP INT0_vect"), ("0x0004", "JMP INT1_vect"),
           ("0x0006", "JMP PCINT0_vect"), ("…", "…"), ("0x0020", "JMP TIMER0_OVF_vect"), ("0x0024", "JMP USART_RX_vect"),
           ("…", "…"), ("0x0032", "JMP SPM_READY_vect")]
    for i, (a, n) in enumerate(avr):
        y = 80 + i * 34
        on = n == "JMP INT0_vect"
        s.rect(110, y, 230, 34, GREEN_BG if on else "#ffffff", GREEN if on else LINE, 0, 1)
        s.text(100, y + 22, a, 12, "end", MUTED)
        s.text(225, y + 22, n, 12, weight="bold" if on else "normal")
    s.text(210, 412, "Flash 맨 앞, 워드 주소. 벡터 26개, 각각 명령 하나(2워드)", 11, color=MUTED)
    s.text(210, 430, "우선순위는 주소가 낮을수록 높다 (고정)", 11, color=MUTED)
    # Cortex-M
    s.text(660, 64, "(나) STM32F411: 주소의 목록", 14, weight="bold", color=BLUE)
    cm = [("0x000", "초기 SP 값"), ("0x004", "Reset_Handler"), ("0x008", "NMI_Handler"), ("0x00C", "HardFault_Handler"),
          ("…", "…"), ("0x03C", "SysTick_Handler"), ("0x040", "IRQ 0: WWDG"), ("…", "…"),
          ("0x0E0", "IRQ 40: EXTI15_10"), ("…", "…"), ("0x194", "IRQ 85: SPI5")]
    for i, (a, n) in enumerate(cm):
        y = 80 + i * 30
        on = "EXTI15_10" in n
        first = i < 2
        s.rect(560, y, 230, 30, BLUE_BG if on else (GRAY_BG if first else "#ffffff"), BLUE if on else LINE, 0, 1)
        s.text(550, y + 20, a, 12, "end", MUTED)
        s.text(675, y + 20, n, 12, weight="bold" if on or first else "normal")
    s.text(660, 430, "0x0000 0000(Flash 별칭)부터 4바이트 주소 102개", 11, color=MUTED)
    s.text(660, 448, "IRQ n의 위치 = 0x40 + n × 4.  우선순위는 NVIC에서 설정", 11, color=MUTED)
    s.save("ch07/fig7-3-vector-table.svg")


def fig_7_4():
    s = Svg(900, 440)
    s.text(450, 30, "그림 7-4  우선순위와 선점: 중첩 인터럽트 타임라인", 16, weight="bold")
    lanes = [("EXTI ISR", "우선순위 1 (높음)", ORANGE_BG, ORANGE), ("TIM2 ISR", "우선순위 2", BLUE_BG, BLUE),
             ("메인", "스레드 모드", GRAY_BG, LINE)]
    ys = [80, 160, 240]
    for (n, sub, f, st), y in zip(lanes, ys):
        s.text(130, y + 14, n, 13, "end", weight="bold")
        s.text(130, y + 30, sub, 10, "end", MUTED)
        s.line(140, y + 36, 870, y + 36, "#dde3ea", 1)
    X = lambda t: 150 + t * 34
    # 메인
    s.rect(X(0), 240, X(3) - X(0), 30, GRAY_BG, LINE, 3, 1)
    s.rect(X(14), 240, X(20) - X(14), 30, GRAY_BG, LINE, 3, 1)
    # TIM2
    s.rect(X(3), 160, X(6) - X(3), 30, BLUE_BG, BLUE, 3, 1)
    s.rect(X(10), 160, X(12) - X(10), 30, BLUE_BG, BLUE, 3, 1)
    # EXTI
    s.rect(X(6), 80, X(10) - X(6), 30, ORANGE_BG, ORANGE, 3, 1)
    # 두번째 TIM2 요청 (같은 우선순위, 대기 후 tail-chain)
    s.rect(X(12), 160, X(14) - X(12), 30, "#dbe7fb", BLUE, 3, 1)
    s.text((X(12) + X(14)) / 2, 180, "다음", 10)
    # 화살표
    for t in (3, 6):
        s.line(X(t), 300, X(t), 290, RED, 1.5)
    s.text(X(3), 316, "① TIM2 요청", 11, color=RED)
    s.text(X(6), 316, "② EXTI 요청 → 선점", 11, color=RED, weight="bold")
    s.text(X(8), 100, "EXTI ISR", 12, weight="bold")
    s.text((X(3) + X(6)) / 2, 180, "TIM2 ISR", 11)
    s.text((X(10) + X(12)) / 2, 180, "이어서", 10)
    s.text(150, 346, "③ EXTI ISR이 실행되는 동안 TIM2 ISR은 멈춘 채 기다린다", 12, "start", MUTED)
    s.text(150, 368, "④ EXTI ISR이 끝나면 TIM2 ISR이 멈춘 곳부터 이어서 실행된다", 12, "start", MUTED)
    s.text(150, 390, "⑤ 그사이 들어온 TIM2 요청은 메인으로 돌아가지 않고 바로 이어서 처리된다 (꼬리 물기)", 12, "start", MUTED)
    s.text(450, 428, "숫자가 작을수록 우선순위가 높다. 더 높은 우선순위만 실행 중인 ISR을 선점할 수 있다", 11, color=INK)
    s.save("ch07/fig7-4-nesting.svg")


def fig_7_5():
    t = Timing(24, 32, 150, 70, "그림 7-5  버튼 바운싱 파형과 디바운싱 처리")
    raw = [(3, "h", None, None), (0.4, "l", None, None), (0.3, "h", None, None), (0.6, "l", None, None),
           (0.3, "h", None, None), (0.3, "l", None, None), (0.2, "h", None, None), (6.9, "l", "눌린 상태", None),
           (0.3, "h", None, None), (0.4, "l", None, None), (0.3, "h", None, None), (11, "h", "뗀 상태", None)]
    t.signal("버튼 핀", raw, sub="실제 신호")
    isr = [(3, "h", None, None), (0.15, "l", None, None), (0.55, "h", None, None), (0.15, "l", None, None),
           (0.75, "h", None, None), (0.15, "l", None, None), (7.75, "h", None, None), (0.15, "l", None, None),
           (11.35, "h", None, None)]
    t.signal("하강 에지 검출", isr, sub="그대로 쓰면 4번")
    clean = [(3, "h", None, None), (9.6, "l", None, None), (11.4, "h", None, None)]
    t.signal("디바운싱 결과", clean, sub="한 번만 인식")
    t.note("span", row=0, t0=3, t1=5.1, label="바운싱 수 ms", color=RED, below=False)
    t.note("span", row=2, t0=3, t1=8, label="첫 에지 뒤 20 ms 동안 무시", color=GREEN)
    t.note("vline", t=3, color=ORANGE)
    t.note("text", t=12, y=290, label="뗄 때의 바운싱에도 하강 에지가 생긴다. 디바운싱은 누를 때와 뗄 때 모두 필요하다", size=12)
    t.render("ch07/fig7-5-debounce.svg", 70)


# ---------------------------------------------------------------------------
# 8장
# ---------------------------------------------------------------------------

def fig_8_1():
    s = Svg(900, 400)
    s.text(450, 30, "그림 8-1  타이머 카운터의 톱니파: 세고, 넘치고, 처음부터", 16, weight="bold")
    x0, y0, w, h = 110, 300, 720, 210      # y0는 0의 y좌표
    s.line(x0, y0, x0 + w + 20, y0, MUTED, 1.5, arrow=True)
    s.line(x0, y0, x0, y0 - h - 30, MUTED, 1.5, arrow=True)
    s.text(x0 + w + 20, y0 + 20, "시간", 12, "end", MUTED)
    s.text(x0 - 10, y0 - h - 34, "CNT", 13, "end", INK, "bold")
    s.text(x0 - 10, y0 + 4, "0", 12, "end", MUTED)
    s.line(x0, y0 - h, x0 + w, y0 - h, ORANGE, 1.2, "5 4")
    s.text(x0 - 10, y0 - h + 4, "ARR", 12, "end", ORANGE, "bold")
    period = 160
    s.line(x0, y0 - h * 0.45, x0 + w, y0 - h * 0.45, BLUE, 1.2, "5 4")
    s.text(x0 - 10, y0 - h * 0.45 + 4, "CCR", 12, "end", BLUE, "bold")
    pts = []
    for k in range(4):
        a = x0 + k * period
        # 계단 모양 톱니파
        steps = 16
        for i in range(steps):
            xa = a + i * period / steps
            ya = y0 - h * i / (steps - 1)
            pts.append(f"L{xa:.1f},{ya:.1f} L{xa + period / steps:.1f},{ya:.1f}")
        # 업데이트 이벤트
        ex = a + period
        s.line(ex, y0 + 6, ex, y0 + 26, RED, 2)
        s.poly([(ex, y0 + 6), (ex - 5, y0 + 15), (ex + 5, y0 + 15)], RED, RED, 1)
        # 비교 일치
        cx = a + period * 0.45
        s.add(f'<circle cx="{cx:.1f}" cy="{y0 - h * 0.45:.1f}" r="5" fill="{BLUE}"/>')
    s.path(f"M{x0},{y0} " + " ".join(pts), INK, 2)
    s.text(x0 + period, y0 + 44, "업데이트 이벤트", 12, color=RED, weight="bold")
    s.text(x0 + period, y0 + 60, "(오버플로, 인터럽트)", 11, color=RED)
    s.text(x0 + period * 0.45 - 8, y0 - h * 0.45 - 16, "비교 일치", 11, "end", BLUE, "bold")
    s.path(f"M{x0 + 2 * period},{y0 + 80} L{x0 + 3 * period},{y0 + 80}", LINE, 1.5)
    s.line(x0 + 2 * period, y0 + 74, x0 + 2 * period, y0 + 86, LINE, 1.5)
    s.line(x0 + 3 * period, y0 + 74, x0 + 3 * period, y0 + 86, LINE, 1.5)
    s.text(x0 + 2.5 * period, y0 + 72, "한 주기 = (ARR + 1) 틱", 12, weight="bold")
    s.save("ch08/fig8-1-counter-sawtooth.svg")


def fig_8_2():
    s = Svg(900, 330)
    s.text(450, 30, "그림 8-2  프리스케일러와 오토리로드로 주기 만들기", 16, weight="bold")
    s.box(30, 80, 150, 70, "타이머 클럭", "16 MHz", GREEN_BG, GREEN, 15)
    s.box(240, 80, 190, 70, "프리스케일러", "÷ (PSC + 1)", BLUE_BG, BLUE, 15)
    s.box(490, 80, 190, 70, "카운터", "0 → ARR, 다시 0", BLUE_BG, BLUE, 15)
    s.box(740, 80, 130, 70, "업데이트", "인터럽트", ORANGE_BG, ORANGE, 15)
    s.line(182, 115, 236, 115, LINE, 2, arrow=True)
    s.line(432, 115, 486, 115, LINE, 2, arrow=True)
    s.line(682, 115, 736, 115, LINE, 2, arrow=True)
    s.text(209, 106, "16 MHz", 11, color=MUTED)
    s.text(459, 106, "틱", 11, color=MUTED)
    # 예시 계산
    s.rect(30, 180, 840, 120, "#fafbfc", MUTED, 10, 1)
    s.text(450, 206, "예: 16 MHz에서 2 Hz(0.5초마다) 인터럽트", 14, weight="bold")
    s.text(60, 236, "PSC = 15999  →  16 MHz ÷ 16000 = 1 kHz  (틱 하나 = 1 ms)", 13, "start")
    s.text(60, 260, "ARR = 499    →  1 kHz ÷ 500 = 2 Hz", 13, "start")
    s.text(60, 284, "업데이트 주파수 = 타이머 클럭 ÷ ((PSC + 1) × (ARR + 1))", 13, "start", BLUE, "bold")
    s.save("ch08/fig8-2-prescaler-arr.svg")


def fig_8_3():
    t = Timing(16, 46, 150, 70, "그림 8-3  듀티비에 따른 PWM 파형 (주기는 같다)")
    for name, on in (("25%", 1), ("50%", 2), ("75%", 3)):
        segs = []
        for _ in range(4):
            segs += [(on, "h", None, None), (4 - on, "l", None, None)]
        t.signal(f"듀티 {name}", segs, sub="평균 전압 " + {"25%": "1.25 V", "50%": "2.5 V", "75%": "3.75 V"}[name])
    t.note("span", row=0, t0=0, t1=4, label="주기 T", color=BLUE, below=False)
    t.note("span", row=2, t0=4, t1=7, label="High 시간", color=ORANGE)
    t.note("text", t=8, y=270, label="듀티비 = High 시간 ÷ 주기.  5 V 출력이면 평균 전압 = 5 V × 듀티비", size=12)
    t.render("ch08/fig8-3-pwm-duty.svg", 60)


def fig_8_4():
    s = Svg(900, 400)
    s.text(450, 30, "그림 8-4  서보 모터 제어 펄스 폭과 각도", 16, weight="bold")
    cases = [(1.0, "0°", -90), (1.5, "90°", 0), (2.0, "180°", 90)]
    x0, scale = 150, 26            # 1 ms = 26 px, 20 ms = 520 px
    for i, (ms, ang, deg) in enumerate(cases):
        y = 80 + i * 100
        s.text(x0 - 20, y + 20, f"{ms} ms", 14, "end", weight="bold")
        pw = ms * scale
        s.path(f"M{x0 - 10},{y + 30} L{x0},{y + 30} L{x0},{y} L{x0 + pw},{y} L{x0 + pw},{y + 30} "
               f"L{x0 + 20 * scale},{y + 30} L{x0 + 20 * scale},{y} L{x0 + 20 * scale + pw},{y} "
               f"L{x0 + 20 * scale + pw},{y + 30} L{x0 + 20 * scale + 40},{y + 30}", INK, 2)
        s.rect(x0, y, pw, 30, ORANGE_BG, "none", 0, 0)
        # 서보 혼
        import math
        cx, cy = 820, y + 22
        s.add(f'<circle cx="{cx}" cy="{cy}" r="26" fill="{GRAY_BG}" stroke="{LINE}" stroke-width="1.5"/>')
        a = math.radians(deg - 90)
        s.line(cx, cy, cx + 24 * math.cos(a), cy + 24 * math.sin(a), ORANGE, 5)
        s.add(f'<circle cx="{cx}" cy="{cy}" r="4" fill="{LINE}"/>')
        s.text(cx - 36, cy + 5, ang, 14, "end", ORANGE, "bold")
    s.line(x0, 365, x0 + 20 * scale, 365, LINE, 1.5, both=True)
    s.text(x0 + 10 * scale, 358, "주기 20 ms (50 Hz)", 12, weight="bold")
    s.text(450, 390, "흔한 취미용 서보의 예. 실제 펄스 범위(예: 0.5~2.5 ms)와 각도는 제품마다 달라서 데이터시트로 확인한다", 11, color=MUTED)
    s.save("ch08/fig8-4-servo.svg")


def fig_8_5():
    s = Svg(900, 400)
    s.text(450, 30, "그림 8-5  입력 캡처로 펄스 폭 측정하기", 16, weight="bold")
    x0, x1 = 140, 860
    # 입력 신호
    y = 80
    s.text(x0 - 14, y + 20, "입력 핀", 13, "end", weight="bold")
    r, f = 300, 560
    s.path(f"M{x0},{y + 30} L{r},{y + 30} L{r},{y} L{f},{y} L{f},{y + 30} L{x1},{y + 30}", INK, 2)
    # 카운터 (톱니 일부: 계속 증가)
    yb, yt = 330, 150
    s.text(x0 - 14, yt + 10, "CNT", 13, "end", weight="bold")
    s.line(x0, yb, x1, yb, MUTED, 1.2)
    s.path(f"M{x0},{yb - 20} L{x1},{yt + 10}", BLUE, 2)
    def cnt_y(x):
        return (yb - 20) + (x - x0) * ((yt + 10) - (yb - 20)) / (x1 - x0)
    for x, lbl, val in ((r, "상승 에지에서 캡처", "CCR = 1200"), (f, "하강 에지에서 캡처", "CCR = 2700")):
        s.line(x, y + 34, x, yb, RED, 1.2, "4 3")
        s.add(f'<circle cx="{x}" cy="{cnt_y(x):.1f}" r="6" fill="{RED}"/>')
        s.text(x + 10, cnt_y(x) + 22, val, 13, "start", RED, "bold")
        s.text(x, y - 8, lbl, 12, color=RED)
    s.line(r, yb + 20, f, yb + 20, ORANGE, 1.5, both=True)
    s.text((r + f) / 2, yb + 40, "펄스 폭 = (2700 − 1200) 틱 = 1500 틱 = 1.5 ms (틱 1 µs일 때)", 13, weight="bold", color=ORANGE)
    s.text(450, 392, "하드웨어가 에지 순간의 카운터 값을 CCR에 복사하므로, ISR이 늦게 실행되어도 측정값은 정확하다", 12, color=MUTED)
    s.save("ch08/fig8-5-input-capture.svg")


# ---------------------------------------------------------------------------
# 9장
# ---------------------------------------------------------------------------

def fig_9_1():
    import math
    s = Svg(900, 420)
    s.text(450, 30, "그림 9-1  아날로그 신호의 샘플링과 양자화 (3비트 ADC의 예)", 16, weight="bold")
    x0, y0, w, h = 120, 340, 700, 260       # y0 = 0 V
    levels = 8
    s.line(x0, y0, x0 + w + 10, y0, MUTED, 1.2, arrow=True)
    s.line(x0, y0, x0, y0 - h - 20, MUTED, 1.2, arrow=True)
    s.text(x0 + w + 10, y0 + 20, "시간", 12, "end", MUTED)
    for k in range(levels):
        y = y0 - (k + 0.5) * h / levels
        s.line(x0, y, x0 + w, y, "#e6eaef", 1)
        s.text(x0 - 10, y + 4, f"{k:03b}", 11, "end", MUTED)
    s.text(x0 - 10, y0 - h - 24, "코드", 12, "end", INK, "bold")

    def f(t):
        return 0.5 + 0.38 * math.sin(2 * math.pi * t * 1.3) + 0.08 * math.sin(2 * math.pi * t * 3.1)
    # 원래 신호
    pts = " ".join(f"L{x0 + i:.1f},{y0 - f(i / w) * h:.1f}" for i in range(0, w + 1, 4))
    s.path(f"M{x0},{y0 - f(0) * h:.1f} " + pts, BLUE, 2)
    # 샘플과 양자화 계단
    n = 20
    step_pts = []
    for i in range(n):
        t = i / n
        x = x0 + t * w
        v = f(t)
        code = min(levels - 1, int(v * levels))
        yq = y0 - (code + 0.5) * h / levels
        step_pts.append(f"L{x:.1f},{yq:.1f} L{x + w / n:.1f},{yq:.1f}")
        s.line(x, y0, x, y0 - v * h, "#c9d2dc", 1, "3 3")
        s.add(f'<circle cx="{x:.1f}" cy="{y0 - v * h:.1f}" r="4" fill="{RED}"/>')
    first = step_pts[0].split()[0][1:]
    s.path(f"M{first} " + " ".join(step_pts), ORANGE, 2.5)
    # 범례
    lx = 600
    s.line(lx, 58, lx + 30, 58, BLUE, 2)
    s.text(lx + 38, 62, "원래 아날로그 신호", 12, "start")
    s.add(f'<circle cx="{lx + 15}" cy="78" r="4" fill="{RED}"/>')
    s.text(lx + 38, 82, "샘플링 (일정한 간격으로 값을 읽음)", 12, "start")
    s.line(lx, 98, lx + 30, 98, ORANGE, 2.5)
    s.text(lx + 38, 102, "양자화 (가장 가까운 단계로 반올림)", 12, "start")
    s.text(450, 400, "비트 수가 많을수록 단계가 촘촘하고, 샘플링이 빠를수록 시간 축이 촘촘하다", 12, color=MUTED)
    s.save("ch09/fig9-1-sampling-quantization.svg")


def fig_9_2():
    import math
    s = Svg(900, 400)
    s.text(450, 30, "그림 9-2  나이퀴스트 주파수와 에일리어싱", 16, weight="bold")

    def panel(y, title, fsig, fs, color):
        x0, w, h = 120, 720, 90
        s.text(x0 - 10, y - 50, title, 13, "start", color, "bold")
        s.line(x0, y, x0 + w, y, "#e6eaef", 1)
        pts = " ".join(f"L{x0 + i:.1f},{y - h / 2 * math.sin(2 * math.pi * fsig * i / w):.1f}" for i in range(0, w + 1, 3))
        s.path(f"M{x0},{y} " + pts, BLUE, 1.6)
        n = int(fs) + 1
        samp = []
        for k in range(n):
            t = k / fs
            if t > 1:
                break
            x = x0 + t * w
            yy = y - h / 2 * math.sin(2 * math.pi * fsig * t)
            samp.append((x, yy))
            s.add(f'<circle cx="{x:.1f}" cy="{yy:.1f}" r="4.5" fill="{RED}"/>')
        return samp, x0, w, h

    panel(130, "(가) 신호 2 Hz, 샘플링 20 Hz: 충분히 빠르다 → 모양이 그대로 남는다", 2, 20, GREEN)
    samp, x0, w, h = panel(300, "(나) 신호 9 Hz, 샘플링 10 Hz: 너무 느리다 → 1 Hz짜리 가짜 신호로 보인다", 9, 10, RED)
    pts = " ".join(f"L{x0 + i:.1f},{300 + h / 2 * math.sin(2 * math.pi * 1 * i / w):.1f}" for i in range(0, w + 1, 3))
    s.path(f"M{x0},{300} " + pts, ORANGE, 2.5)
    s.text(450, 388, "샘플링 주파수는 신호의 가장 높은 주파수의 2배보다 커야 한다 (나이퀴스트). 넘는 성분은 아날로그 필터로 미리 없앤다", 12, color=MUTED)
    s.save("ch09/fig9-2-aliasing.svg")


def fig_9_3():
    s = Svg(900, 450)
    s.text(450, 30, "그림 9-3  SAR ADC의 변환 과정: 이진 탐색 (3비트, Vref = 8 V, 입력 5.3 V)", 16, weight="bold")
    x0, y0, h = 130, 360, 280     # 0~8 V
    def Y(v):
        return y0 - v / 8 * h
    s.line(x0, y0, x0, y0 - h - 10, MUTED, 1.2, arrow=True)
    for v in range(0, 9):
        s.line(x0 - 5, Y(v), x0, Y(v), MUTED, 1.2)
        s.text(x0 - 10, Y(v) + 4, f"{v} V", 11, "end", MUTED)
    s.line(x0, Y(5.3), 640, Y(5.3), RED, 2, "6 4")
    s.text(x0 + 8, Y(5.3) - 8, "입력 5.3 V", 12, "start", RED, "bold")
    steps = [("1단계: 4 V와 비교", 4, "크다 → 비트2 = 1", "1??"),
             ("2단계: 6 V와 비교", 6, "작다 → 비트1 = 0", "10?"),
             ("3단계: 5 V와 비교", 5, "크다 → 비트0 = 1", "101")]
    for i, (t, v, r, code) in enumerate(steps):
        x = 200 + i * 150
        s.line(x - 40, Y(v), x + 40, Y(v), BLUE, 3)
        s.text(x + 46, Y(v) + 18, f"{v} V", 12, "start", BLUE, "bold")
        s.text(x, 70, t, 12, weight="bold")
        s.text(x, 88, r, 11, color=MUTED)
        s.rect(x - 30, y0 + 16, 60, 28, ORANGE_BG, ORANGE, 4, 1.2)
        s.text(x, y0 + 35, code, 14, weight="bold")
        if i < 2:
            s.line(x + 45, y0 + 30, x + 105, y0 + 30, LINE, 1.5, arrow=True)
    s.rect(680, 150, 190, 120, "#fafbfc", MUTED, 8, 1)
    s.text(775, 176, "결과 101₂ = 5", 14, weight="bold")
    s.text(775, 200, "5 × (8 V ÷ 8) = 5 V", 12, color=MUTED)
    s.text(775, 222, "~ 6 V 사이", 12, color=MUTED)
    s.text(775, 248, "n비트 = 비교 n번", 12, color=BLUE, weight="bold")
    s.text(450, 438, "비교할 때마다 남은 범위를 절반으로 줄인다. 그동안 입력은 샘플-홀드 커패시터에 붙잡아 둔다", 12, color=MUTED)
    s.save("ch09/fig9-3-sar.svg")


def fig_9_4():
    s = Svg(820, 380)
    s.text(410, 30, "그림 9-4  가변저항 분압 회로", 16, weight="bold")
    x = 200
    supply(s, x, 80, "VCC (5 V 또는 3.3 V)")
    s.line(x, 80, x, 110, LINE, 2)
    s.rect(x - 14, 110, 28, 160, "#ffffff", LINE, 2, 2)
    s.text(x - 24, 150, "R1", 13, "end", weight="bold")
    s.text(x - 24, 240, "R2", 13, "end", weight="bold")
    # 와이퍼
    s.poly([(x + 16, 190), (x + 34, 182), (x + 34, 198)], ORANGE, ORANGE, 1)
    s.line(x + 34, 190, 360, 190, ORANGE, 2.5)
    s.text(300, 182, "와이퍼", 11, color=ORANGE)
    s.line(x, 270, x, 300, LINE, 2)
    ground(s, x, 300)
    s.box(360, 165, 130, 50, "ADC 입력", "A0 / PA0", BLUE_BG, BLUE, 13)
    s.rect(540, 110, 250, 150, "#fafbfc", MUTED, 8, 1)
    s.text(665, 138, "Vout = VCC × R2 ÷ (R1 + R2)", 13, weight="bold")
    s.text(665, 166, "손잡이를 돌리면 R1과 R2의 비율이 바뀐다", 12, color=MUTED)
    s.text(665, 194, "ADC 기준 전압 = VCC이면", 12, color=MUTED)
    s.text(665, 216, "ADC 값 = 최대 코드 × R2 ÷ (R1 + R2)", 12, weight="bold", color=BLUE)
    s.text(665, 240, "→ 전원 전압이 흔들려도 값은 그대로", 12, color=BLUE)
    s.text(410, 360, "가변저항은 10 kΩ 정도를 쓴다. 너무 크면 ADC 입력의 샘플링 커패시터를 제때 채우지 못한다", 12, color=MUTED)
    s.save("ch09/fig9-4-pot-divider.svg")


def fig_9_5():
    import math, random
    s = Svg(900, 380)
    s.text(450, 30, "그림 9-5  이동 평균 필터 적용 전후", 16, weight="bold")
    random.seed(7)
    x0, y0, w, h = 110, 320, 740, 240
    s.line(x0, y0, x0 + w + 10, y0, MUTED, 1.2, arrow=True)
    s.line(x0, y0, x0, y0 - h - 10, MUTED, 1.2, arrow=True)
    s.text(x0 - 10, y0 - h - 14, "ADC 값", 12, "end", INK, "bold")
    s.text(x0 + w + 10, y0 + 20, "샘플 번호", 12, "end", MUTED)
    n = 120
    truth = [0.45 + (0.25 if 40 <= i < 85 else 0.0) for i in range(n)]
    raw = [t + random.gauss(0, 0.06) for t in truth]
    N = 8
    avg = []
    for i in range(n):
        win = raw[max(0, i - N + 1): i + 1]
        avg.append(sum(win) / len(win))
    def P(arr, col, wd):
        pts = " ".join(f"L{x0 + i * w / (n - 1):.1f},{y0 - v * h:.1f}" for i, v in enumerate(arr))
        s.path(f"M{x0},{y0 - arr[0] * h:.1f} " + pts, col, wd)
    P(raw, "#9fb3c8", 1.3)
    P(avg, ORANGE, 2.5)
    lx = 620
    s.line(lx, 60, lx + 30, 60, "#9fb3c8", 1.5)
    s.text(lx + 38, 64, "원래 값 (잡음 섞임)", 12, "start")
    s.line(lx, 82, lx + 30, 82, ORANGE, 2.5)
    s.text(lx + 38, 86, "최근 8개 평균", 12, "start")
    ax = x0 + 44 * w / (n - 1)
    s.text(ax - 30, y0 - 0.98 * h, "평균은 계단을 늦게 따라간다 (지연)", 12, "start", ORANGE, "bold")
    s.line(ax - 10, y0 - 0.95 * h, ax, y0 - 0.62 * h, ORANGE, 1.5, arrow=True)
    s.text(450, 362, "잡음은 줄지만 변화에 늦게 반응한다. 창이 길수록 더 매끄럽고 더 느리다", 12, color=MUTED)
    s.save("ch09/fig9-5-moving-average.svg")


# ---------------------------------------------------------------------------
# 10장
# ---------------------------------------------------------------------------

def fig_10_1():
    s = Svg(900, 400)
    s.text(450, 30, "그림 10-1  CPU 복사와 DMA 전송의 비교", 16, weight="bold")
    x0, x1 = 170, 860
    # (가) CPU 복사
    s.text(30, 76, "(가) CPU가 복사", 14, "start", RED, "bold")
    s.text(160, 106, "CPU", 13, "end", weight="bold")
    s.text(160, 140, "버스", 13, "end", weight="bold")
    k = 0
    x = x0
    while x < x1 - 40:
        s.rect(x, 90, 30, 24, BLUE_BG, BLUE, 2, 1)
        s.text(x + 15, 107, "읽기", 9)
        s.rect(x + 32, 90, 30, 24, ORANGE_BG, ORANGE, 2, 1)
        s.text(x + 47, 107, "쓰기", 9)
        s.rect(x, 126, 62, 22, GRAY_BG, LINE, 2, 1)
        x += 66
        k += 1
    s.text(515, 172, "복사하는 동안 CPU는 다른 일을 못 한다. 워드 하나마다 명령 여러 개", 12, color=MUTED)
    # (나) DMA
    s.text(30, 222, "(나) DMA가 전송", 14, "start", GREEN, "bold")
    s.text(160, 252, "CPU", 13, "end", weight="bold")
    s.text(160, 286, "DMA", 13, "end", weight="bold")
    s.text(160, 320, "버스", 13, "end", weight="bold")
    s.rect(x0, 236, 50, 24, BLUE_BG, BLUE, 2, 1)
    s.text(x0 + 25, 253, "설정", 10)
    s.rect(x0 + 54, 236, x1 - x0 - 120, 24, GREEN_BG, GREEN, 2, 1)
    s.text((x0 + 54 + x1 - 66) / 2, 253, "CPU는 다른 계산을 계속한다 (또는 잠든다)", 11)
    s.rect(x1 - 62, 236, 52, 24, ORANGE_BG, ORANGE, 2, 1)
    s.text(x1 - 36, 253, "완료 ISR", 9)
    x = x0 + 54
    while x < x1 - 70:
        s.rect(x, 272, 26, 24, GRAY_BG, LINE, 2, 1)
        x += 30
    s.text(x0 + 54 + 13, 289, "R/W", 8)
    x = x0 + 54
    while x < x1 - 70:
        s.rect(x, 306, 26, 22, "#dde3ea", LINE, 2, 1)
        x += 30
    s.text(515, 352, "DMA가 버스를 쓰는 사이사이에 CPU도 버스를 쓴다. 다른 메모리라면 동시에 진행된다 (버스 매트릭스)", 12, color=MUTED)
    s.text(515, 374, "데이터 하나를 옮기는 데 CPU 명령이 하나도 들지 않는다", 12, color=GREEN, weight="bold")
    s.save("ch10/fig10-1-cpu-vs-dma.svg")


def fig_10_2():
    s = Svg(900, 450)
    s.text(450, 30, "그림 10-2  DMA 스트림과 채널: STM32F411 DMA2 스트림 0의 요청 연결", 16, weight="bold")
    s.text(140, 66, "주변장치 요청", 13, weight="bold", color=ORANGE)
    reqs = [("CH0", "ADC1"), ("CH1", "-"), ("CH2", "-"), ("CH3", "SPI1_RX"), ("CH4", "SPI4_RX"), ("CH5", "-"),
            ("CH6", "TIM1_TRIG"), ("CH7", "-")]
    for i, (c, n) in enumerate(reqs):
        y = 80 + i * 40
        on = n == "ADC1"
        s.rect(60, y, 160, 32, ORANGE_BG if on else "#ffffff", ORANGE if on else MUTED, 4, 1.5 if on else 1)
        s.text(72, y + 21, c, 12, "start", weight="bold")
        s.text(210, y + 21, n, 12, "end", INK if n != "-" else MUTED)
        s.line(222, y + 16, 300, 240, ORANGE if on else "#dde3ea", 2 if on else 1)
    s.poly([(300, 120), (350, 150), (350, 330), (300, 360)], GRAY_BG, LINE, 2)
    s.text(325, 235, "CHSEL", 10, weight="bold")
    s.text(325, 382, "스트림 0의 선택기", 11, color=MUTED)
    s.line(352, 240, 420, 240, ORANGE, 2.5, arrow=True)
    s.rect(422, 160, 210, 160, BLUE_BG, BLUE, 10, 2)
    s.text(527, 186, "DMA2 스트림 0", 14, weight="bold")
    for i, t in enumerate(["PAR: 주변장치 주소", "M0AR: 메모리 주소", "NDTR: 남은 개수", "CR: 방향 · 크기 · 증가 · 원형"]):
        s.text(440, 214 + i * 24, t, 12, "start")
    s.line(634, 240, 700, 240, LINE, 2, both=True)
    s.box(702, 200, 170, 80, "버스 매트릭스", "→ ADC1_DR, SRAM", GRAY_BG, LINE, 13)
    s.text(450, 420, "스트림마다 8개 채널 중 하나를 고른다. 어느 주변장치가 어느 스트림·채널에 있는지는 RM0383의 DMA 요청 표에서 찾는다", 12, color=MUTED)
    s.text(450, 440, "ADC1은 DMA2의 스트림 0 또는 스트림 4, 채널 0", 12, color=MUTED)
    s.save("ch10/fig10-2-dma-streams.svg")


def fig_10_3():
    s = Svg(900, 440)
    s.text(450, 30, "그림 10-3  원형 버퍼와 하프/풀 전송 인터럽트 (더블 버퍼링)", 16, weight="bold")
    # 버퍼
    bx, by, bw = 120, 70, 660
    s.rect(bx, by, bw / 2, 44, BLUE_BG, BLUE, 0, 1.5)
    s.rect(bx + bw / 2, by, bw / 2, 44, ORANGE_BG, ORANGE, 0, 1.5)
    s.text(bx + bw / 4, by + 28, "앞쪽 절반 buf[0 ~ 31]", 13, weight="bold")
    s.text(bx + 3 * bw / 4, by + 28, "뒤쪽 절반 buf[32 ~ 63]", 13, weight="bold")
    s.path(f"M{bx + bw},{by + 22} L{bx + bw + 30},{by + 22} L{bx + bw + 30},{by + 64} L{bx - 30},{by + 64} "
           f"L{bx - 30},{by + 22} L{bx - 4},{by + 22}", MUTED, 1.5, arrow=True)
    s.text(450, by + 82, "끝에 도달하면 처음으로 돌아가 계속 채운다 (원형 모드, CIRC = 1)", 12, color=MUTED)
    # 타임라인
    x0, unit = 160, 170
    s.text(150, 214, "DMA가 채우는 곳", 12, "end", weight="bold")
    s.text(150, 284, "CPU가 처리하는 곳", 12, "end", weight="bold")
    for i in range(4):
        x = x0 + i * unit
        front = i % 2 == 0
        s.rect(x, 196, unit - 6, 30, BLUE_BG if front else ORANGE_BG, BLUE if front else ORANGE, 3, 1.2)
        s.text(x + unit / 2 - 3, 216, "앞쪽 절반" if front else "뒤쪽 절반", 12)
        # 인터럽트
        ex = x + unit - 3
        s.line(ex, 180, ex, 312, RED, 1.2, "4 3")
        s.text(ex, 174, "HT" if front else "TC", 12, color=RED, weight="bold")
        if i < 3:
            s.rect(ex + 3, 266, unit - 12, 30, BLUE_BG if front else ORANGE_BG, BLUE if front else ORANGE, 3, 1.2)
            s.text(ex + unit / 2 - 3, 286, "앞쪽 처리" if front else "뒤쪽 처리", 12)
    s.text(450, 340, "HT(하프 전송): 앞쪽 절반이 찼다 → CPU가 앞쪽을 처리하는 동안 DMA는 뒤쪽을 채운다", 12, color=INK)
    s.text(450, 362, "TC(전송 완료): 뒤쪽 절반이 찼다 → CPU가 뒤쪽을 처리하는 동안 DMA는 다시 앞쪽을 채운다", 12, color=INK)
    s.text(450, 396, "조건: CPU가 절반을 처리하는 시간 < DMA가 나머지 절반을 채우는 시간", 13, color=RED, weight="bold")
    s.text(450, 420, "이 조건을 어기면 처리 중인 데이터를 DMA가 덮어쓴다", 12, color=MUTED)
    s.save("ch10/fig10-3-circular-ht-tc.svg")


# ---------------------------------------------------------------------------
# 11장
# ---------------------------------------------------------------------------

def fig_11_1():
    import math
    s = Svg(900, 440)
    s.text(450, 30, "그림 11-1  저전력 모드별 소비 전류와 깨어나는 시간 (STM32F411, 자릿수 비교)", 16, weight="bold")
    modes = [("Run", "100 MHz", 10000, "수 mA ~ 십수 mA", "-", BLUE),
             ("Sleep", "CPU만 멈춤", 3000, "수 mA", "수 클럭", BLUE),
             ("Stop", "모든 클럭 멈춤", 40, "수십 µA", "수 µs ~ 수십 µs", ORANGE),
             ("Stop + Flash 절전", "", 10, "약 10 µA", "수십 µs ~ 100 µs대", ORANGE),
             ("Standby", "1.2 V 영역 꺼짐", 2, "수 µA", "리셋처럼 재시작", RED)]
    x0, y0, h = 240, 70, 52
    lo, hi = math.log10(1), math.log10(20000)
    W = 400
    for i, (n, sub, ua, lbl, wake, col) in enumerate(modes):
        y = y0 + i * (h + 14)
        s.text(x0 - 14, y + 22, n, 14, "end", weight="bold")
        if sub:
            s.text(x0 - 14, y + 40, sub, 11, "end", MUTED)
        w = W * (math.log10(ua) - lo) / (hi - lo)
        s.rect(x0, y + 6, max(w, 6), h - 12, {BLUE: BLUE_BG, ORANGE: ORANGE_BG, RED: "#fdecec"}[col], col, 4, 1.5)
        s.text(x0 + max(w, 6) + 10, y + 32, lbl, 12, "start", col, "bold")
        s.text(860, y + 32, wake, 12, "end", INK)
    s.text(x0 + W / 2, y0 - 8, "소비 전류 (로그 눈금)", 12, color=MUTED)
    s.text(860, y0 - 8, "깨어나는 시간", 12, "end", MUTED)
    s.text(450, 420, "아래로 갈수록 전류는 자릿수 단위로 줄고, 깨어나는 데 걸리는 시간과 잃는 것(클럭 설정, SRAM)은 늘어난다", 12, color=MUTED)
    s.text(450, 437, "정확한 값은 DS10314의 전류 소비 표에서 조건(전압, 온도, 클럭, 켜 둔 주변장치)과 함께 확인한다", 11, color=MUTED)
    s.save("ch11/fig11-1-lowpower-modes.svg")


def fig_11_2():
    s = Svg(900, 400)
    s.text(450, 30, "그림 11-2  동작–슬립 반복 시 전류 프로파일과 평균 전류", 16, weight="bold")
    x0, y0, w = 110, 300, 740
    s.line(x0, y0, x0 + w + 10, y0, MUTED, 1.2, arrow=True)
    s.line(x0, y0, x0, 60, MUTED, 1.2, arrow=True)
    s.text(x0 - 10, 66, "전류", 12, "end", INK, "bold")
    s.text(x0 + w + 10, y0 + 20, "시간", 12, "end", MUTED)
    yh, yl = 90, 290
    period = 200
    d = 24
    pts = [f"M{x0},{yl}"]
    for k in range(3):
        a = x0 + 20 + k * period
        pts.append(f"L{a},{yl} L{a},{yh} L{a + d},{yh} L{a + d},{yl}")
    pts.append(f"L{x0 + w},{yl}")
    s.path(" ".join(pts), BLUE, 2.5)
    s.text(x0 + 20 + d / 2, yh - 10, "동작 10 mA", 12, color=BLUE, weight="bold")
    s.text(x0 + 20 + d + 80, yl - 10, "슬립 10 µA", 12, color=BLUE, weight="bold")
    yavg = 262
    s.line(x0, yavg, x0 + w, yavg, ORANGE, 2, "6 4")
    s.text(x0 + w, yavg - 8, "평균 전류", 12, "end", ORANGE, "bold")
    a = x0 + 20 + period
    s.line(a, y0 + 16, a + d, y0 + 16, LINE, 1.2, both=True)
    s.text(a + d / 2, y0 + 34, "20 ms", 11)
    s.line(x0 + 20, y0 + 50, x0 + 20 + period, y0 + 50, LINE, 1.2, both=True)
    s.text(x0 + 20 + period / 2, y0 + 68, "주기 10 s (그림은 비율이 다르다)", 11)
    s.rect(578, 70, 292, 120, "#fafbfc", MUTED, 8, 1)
    s.text(724, 94, "평균 = (10 mA × 0.02 s + 0.01 mA × 9.98 s) ÷ 10 s", 11)
    s.text(724, 116, "= (0.2 + 0.0998) ÷ 10 ≈ 0.030 mA = 30 µA", 12, weight="bold")
    s.text(724, 144, "CR2032 (220 mAh) ÷ 30 µA ≈ 7300 시간", 11)
    s.text(724, 166, "≈ 300일 (자체 방전·온도 여유 전)", 12, weight="bold", color=ORANGE)
    s.save("ch11/fig11-2-current-profile.svg")


def fig_11_3():
    s = Svg(900, 400)
    s.text(450, 30, "그림 11-3  전류 측정 연결 방법", 16, weight="bold")
    # (가) 멀티미터
    s.text(220, 66, "(가) 멀티미터를 직렬로 (평균 전류)", 14, weight="bold", color=BLUE)
    s.box(60, 100, 100, 60, "전원", "3.3 V", GREEN_BG, GREEN, 14)
    s.add(f'<circle cx="225" cy="130" r="28" fill="#ffffff" stroke="{BLUE}" stroke-width="2"/>')
    s.text(225, 136, "A", 18, weight="bold", color=BLUE)
    s.box(290, 100, 110, 60, "MCU", "VDD", BLUE_BG, BLUE, 14)
    s.line(162, 130, 197, 130, LINE, 2)
    s.line(253, 130, 288, 130, LINE, 2)
    s.text(225, 186, "NUCLEO: JP6(IDD) 점퍼를 빼고", 11, color=MUTED)
    s.text(225, 202, "그 자리에 멀티미터를 연결", 11, color=MUTED)
    s.text(225, 236, "빠른 변화는 못 보고 평균만 보인다", 12, color=RED)
    s.text(225, 254, "µA 범위의 내부 저항(버든 전압)으로", 11, color=MUTED)
    s.text(225, 270, "깨어날 때 전압이 떨어져 리셋될 수도 있다", 11, color=MUTED)
    # (나) 션트 + 오실로스코프
    s.text(670, 66, "(나) 션트 저항 + 오실로스코프 (파형)", 14, weight="bold", color=ORANGE)
    s.box(500, 100, 100, 60, "전원", "3.3 V", GREEN_BG, GREEN, 14)
    resistor_h(s, 602, 700, 130, "션트 10 Ω")
    s.box(700, 100, 110, 60, "MCU", "VDD", BLUE_BG, BLUE, 14)
    dot(s, 630, 130)
    dot(s, 680, 130)
    s.line(630, 130, 630, 210, ORANGE, 1.5)
    s.line(680, 130, 680, 210, ORANGE, 1.5)
    s.box(590, 210, 130, 50, "오실로스코프", "차동 측정", ORANGE_BG, ORANGE, 13)
    s.text(655, 290, "I = 션트 양단 전압 ÷ R", 12, weight="bold")
    s.text(655, 310, "10 mA × 10 Ω = 100 mV, 10 µA × 10 Ω = 0.1 mV", 11, color=MUTED)
    s.text(655, 330, "→ 범위가 넓으면 한 저항으로 둘 다 재기 어렵다", 11, color=RED)
    s.rect(60, 340, 400, 46, GRAY_BG, MUTED, 8, 1)
    s.text(260, 360, "전용 장비: 전류를 넓은 범위로 연속 기록한다", 12, weight="bold")
    s.text(260, 378, "(전력 프로파일러, 저전력 측정용 확장 보드 등)", 11, color=MUTED)
    s.save("ch11/fig11-3-current-measurement.svg")


# ---------------------------------------------------------------------------
# 14장 UART
# ---------------------------------------------------------------------------

def fig_14_1():
    s = Svg(760, 230)
    s.text(380, 30, "그림 14-1  UART 연결: TX와 RX는 서로 엇갈려 연결한다", 16, weight="bold")
    s.box(60, 60, 200, 140, "장치 A", "예: STM32", BLUE_BG, BLUE)
    s.box(500, 60, 200, 140, "장치 B", "예: GPS 모듈", ORANGE_BG, ORANGE)
    for y, l, r in ((90, "TX", "TX"), (130, "RX", "RX"), (175, "GND", "GND")):
        s.text(240, y + 5, l, 13, "end", weight="bold")
        s.text(520, y + 5, r, 13, "start", weight="bold")
    s.line(262, 90, 498, 130, BLUE, 2.5, arrow=True)
    s.line(498, 90, 262, 130, ORANGE, 2.5, arrow=True)
    s.line(262, 175, 498, 175, INK, 2.5)
    s.text(380, 165, "GND 공유 (기준 전압을 맞춘다)", 12, color=MUTED)
    s.save("ch14/fig14-1-uart-wiring.svg")


def fig_14_2():
    t = Timing(13, 54, 90, 95, "그림 14-2  UART 프레임 구조 (데이터 8비트 + 패리티 + 정지 1비트)")
    segs = [(1, "h", "유휴", None), (1, "l", None, None)]
    segs += [(1, "d", f"D{i}", BLUE_BG) for i in range(8)]
    segs += [(1, "d", "P", ORANGE_BG), (1, "h", None, None), (1, "h", "유휴", None)]
    t.signal("TX 선", segs)
    t.note("span", row=0, t0=1, t1=2, label="시작 비트", color=GREEN)
    t.note("span", row=0, t0=2, t1=10, label="데이터 8비트 (LSB 먼저)", color=BLUE)
    t.note("span", row=0, t0=10, t1=11, label="패리티", color=ORANGE)
    t.note("span", row=0, t0=11, t1=12, label="정지", color=GREEN)
    t.note("text", t=1.5, y=62, label="1 → 0 하강이 프레임의 시작을 알림", size=12, anchor="start")
    t.render("ch14/fig14-2-uart-frame.svg", 20)


def fig_14_3():
    # 'A' = 0x41 = 0b0100_0001, LSB 먼저: 1 0 0 0 0 0 1 0
    bits = "11" + "0" + "10000010" + "1" + "1"
    labels = {0: "유휴", 1: "", 2: "시작"}
    for i, b in enumerate("10000010"):
        labels[3 + i] = f"D{i}={b}"
    labels[11] = "정지"
    t = Timing(len(bits), 60, 90, 70, "그림 14-3  문자 'A'(0x41)를 8N1, 9600bps로 보낼 때의 파형")
    t.bits("TX", bits, labels=labels)
    for i in range(2, 13):
        t.note("vline", t=i)
    t.note("span", row=0, t0=2, t1=3, label="1비트 = 1/9600 s ≈ 104 µs", color=ORANGE)
    t.note("text", t=7, y=168, label="0x41 = 0100 0001₂ 이지만 선 위에는 LSB부터 1,0,0,0,0,0,1,0 순서로 나간다", size=13)
    t.render("ch14/fig14-3-uart-char-a.svg", 70)


def fig_14_4():
    t = Timing(3, 260, 130, 95, "그림 14-4  16배 오버샘플링: 비트 한가운데에서 값을 읽는다")
    t.signal("RX", [(0.35, "h", "유휴", None), (1, "l", "시작 비트", None), (1, "h", "D0 = 1", None), (0.65, "l", None, None)])
    t.signal("샘플 클럭", [(3, "z", None, None)], sub="보레이트 × 16")
    ts = [0.35 + k / 16 for k in range(0, 40)]
    t.note("sample", row=1, ts=ts, color=MUTED)
    t.note("sample", row=0, ts=[0.35 + 7 / 16, 0.35 + 8 / 16, 0.35 + 9 / 16,
                               1.35 + 7 / 16, 1.35 + 8 / 16, 1.35 + 9 / 16], color=RED)
    t.note("vline", t=0.35, label="하강 감지", dy=26)
    t.note("vline", t=0.35 + 8 / 16, label="8번째 샘플", color=RED, dy=26)
    t.note("vline", t=1.35 + 8 / 16, label="24번째 샘플", color=RED, dy=26)
    t.note("text", t=1.5, y=225, label="빨간 표시: 비트 중앙의 7·8·9번째 샘플을 다수결로 판정해 노이즈에 강해진다", size=13)
    t.render("ch14/fig14-4-oversampling.svg", 70)


def fig_14_6():
    s = Svg(760, 300)
    s.text(380, 30, "그림 14-6  링 버퍼: ISR은 head에 쓰고, 메인 루프는 tail에서 읽는다", 16, weight="bold")
    n, x0, y0, w = 10, 80, 110, 60
    data = {2: "'H'", 3: "'E'", 4: "'L'", 5: "'L'", 6: "'O'"}
    for i in range(n):
        filled = i in data
        s.rect(x0 + i * w, y0, w, 56, BLUE_BG if filled else "#ffffff", BLUE if filled else MUTED, 4)
        s.text(x0 + i * w + w / 2, y0 + 34, data.get(i, ""), 15, weight="bold")
        s.text(x0 + i * w + w / 2, y0 + 78, str(i), 12, color=MUTED)
    tx = x0 + 2 * w + w / 2
    hx = x0 + 7 * w + w / 2
    s.line(tx, 70, tx, 104, GREEN, 2, arrow=True)
    s.text(tx, 62, "tail (다음에 읽을 위치)", 13, color=GREEN, weight="bold")
    s.line(hx, 70, hx, 104, ORANGE, 2, arrow=True)
    s.text(hx, 62, "head (다음에 쓸 위치)", 13, color=ORANGE, weight="bold")
    s.path(f"M{x0 + n * w - 10},{y0 + 92} Q{x0 + n * w + 30},{y0 + 140} {x0 + n * w / 2},{y0 + 150} Q{x0 - 30},{y0 + 140} {x0 + 10},{y0 + 98}", MUTED, 1.5)
    s.line(x0 + 14, y0 + 104, x0 + 10, y0 + 96, MUTED, 1.5, arrow=True)
    s.text(380, y0 + 172, "끝에 닿으면 0번으로 돌아간다   ·   head == tail 이면 비어 있음", 12, color=MUTED)
    s.save("ch14/fig14-6-ring-buffer.svg")


# ---------------------------------------------------------------------------
# 15장 I2C
# ---------------------------------------------------------------------------

def fig_15_1():
    s = Svg(820, 300)
    s.text(410, 30, "그림 15-1  I2C 버스: 선 두 개에 여러 장치가 매달린다", 16, weight="bold")
    ysda, yscl = 120, 150
    s.line(60, ysda, 780, ysda, BLUE, 3)
    s.line(60, yscl, 780, yscl, ORANGE, 3)
    s.text(790, ysda + 5, "SDA", 13, "start", BLUE, "bold")
    s.text(790, yscl + 5, "SCL", 13, "start", ORANGE, "bold")
    # 풀업
    s.text(120, 52, "VDD (3.3 V)", 12, color=MUTED)
    s.line(80, 60, 160, 60, INK, 2)
    for x, y in ((100, ysda), (140, yscl)):
        s.line(x, 60, x, 72, INK, 2)
        s.rect(x - 7, 72, 14, 30, "#ffffff", INK, 2)
        s.line(x, 102, x, y, INK, 2)
        s.add(f'<circle cx="{x}" cy="{y}" r="4" fill="{INK}"/>')
    s.text(165, 92, "풀업 저항 (예: 4.7 kΩ)", 12, "start", MUTED)
    devs = [("마스터", "STM32", BLUE_BG, BLUE), ("슬레이브", "온도 센서 0x76", "#ffffff", LINE),
            ("슬레이브", "EEPROM 0x50", "#ffffff", LINE), ("슬레이브", "OLED 0x3C", "#ffffff", LINE)]
    for i, (a, b, f, st) in enumerate(devs):
        x = 200 + i * 145
        s.box(x, 200, 125, 64, a, b, f, st, 14)
        s.line(x + 45, ysda, x + 45, 198, BLUE, 2)
        s.line(x + 80, yscl, x + 80, 198, ORANGE, 2)
        s.add(f'<circle cx="{x + 45}" cy="{ysda}" r="4" fill="{BLUE}"/>')
        s.add(f'<circle cx="{x + 80}" cy="{yscl}" r="4" fill="{ORANGE}"/>')
    s.text(410, 290, "모든 장치는 선을 0으로 끌어내릴 수만 있고(오픈 드레인), 1은 풀업 저항이 만든다", 12, color=MUTED)
    s.save("ch15/fig15-1-i2c-bus.svg")


def fig_15_3_4():
    # 시작 조건 → 주소 7비트(0x76=1110110) + W(0) → ACK → 정지 조건
    addr = "1110110"
    bits = addr + "0"
    t = Timing(13, 54, 90, 70, "그림 15-4  주소 바이트(0x76, 쓰기) 전송과 ACK")
    # SDA: 유휴 H, 시작(SCL High 동안 하강), 데이터 비트, ACK(슬레이브가 Low), 정지(SCL High 동안 상승)
    sda = [(1, "h", None, None), (0.5, "l", None, None)]
    for i, b in enumerate(bits):
        sda.append((1, "h" if b == "1" else "l", (f"A{6 - i}" if i < 7 else "W"), None))
    sda += [(1, "l", "ACK", GREEN), (1, "l", None, None), (1.5, "h", None, None)]
    t.signal("SDA", sda)
    # SCL은 비트 칸의 가운데에서 High가 되고, 칸 경계(SDA가 바뀌는 곳)에서는 Low다
    scl = [(1.25, "h", None, None), (0.5, "l", None, None)]
    for _ in range(9):
        scl += [(0.5, "h", None, None), (0.5, "l", None, None)]
    scl += [(0.25, "l", None, None), (2.0, "h", None, None)]
    t.signal("SCL", scl)
    t.note("vline", t=1.0, label="시작(S)", color=GREEN)
    t.note("vline", t=11.5, label="정지(P)", color=GREEN)
    t.note("span", row=1, t0=1.5, t1=8.5, label="7비트 주소 0x76 (MSB 먼저)", color=BLUE)
    t.note("span", row=1, t0=8.5, t1=9.5, label="R/W", color=ORANGE)
    t.note("span", row=1, t0=9.5, t1=10.5, label="ACK", color=GREEN)
    t.note("text", t=6.5, y=215, label="SDA는 SCL이 Low일 때만 바뀌고, SCL이 High인 동안 읽힌다. 예외가 시작·정지 조건이다.", size=13)
    t.render("ch15/fig15-4-i2c-address-byte.svg", 80)


def fig_15_6():
    s = Svg(900, 270)
    s.text(450, 30, "그림 15-6  레지스터 읽기 트랜잭션: 쓰기로 주소를 알려 주고, 반복 시작 후 읽는다", 16, weight="bold")
    cells = [("S", 34, GREEN_BG, GREEN), ("슬레이브 주소", 120, BLUE_BG, BLUE), ("W", 34, BLUE_BG, BLUE),
             ("A", 34, ORANGE_BG, ORANGE), ("레지스터 번호", 120, BLUE_BG, BLUE), ("A", 34, ORANGE_BG, ORANGE),
             ("Sr", 38, GREEN_BG, GREEN), ("슬레이브 주소", 120, BLUE_BG, BLUE), ("R", 34, BLUE_BG, BLUE),
             ("A", 34, ORANGE_BG, ORANGE), ("데이터", 110, ORANGE_BG, ORANGE), ("N", 34, BLUE_BG, BLUE),
             ("P", 34, GREEN_BG, GREEN)]
    x = 50
    for lbl, w, f, st in cells:
        s.rect(x, 80, w, 50, f, st, 3)
        s.text(x + w / 2, 110, lbl, 13, weight="bold")
        x += w
    s.text(50 + 34 + 60, 150, "7비트", 11, color=MUTED)
    s.text(50 + 34 + 120 + 68 + 60, 150, "8비트", 11, color=MUTED)
    s.rect(60, 180, 18, 18, BLUE_BG, BLUE, 3)
    s.text(86, 194, "마스터가 보냄", 12, "start")
    s.rect(200, 180, 18, 18, ORANGE_BG, ORANGE, 3)
    s.text(226, 194, "슬레이브가 보냄", 12, "start")
    s.rect(350, 180, 18, 18, GREEN_BG, GREEN, 3)
    s.text(376, 194, "버스 조건 (시작 · 반복 시작 · 정지)", 12, "start")
    s.text(60, 230, "A = ACK(0),  N = NACK(1): 마스터가 \"이제 그만 보내라\"고 알리는 마지막 응답", 12, "start", MUTED)
    s.text(60, 252, "예) 온도 센서 0x76의 0xFA번 레지스터 읽기 →  S 0x76 W A 0xFA A Sr 0x76 R A [값] N P", 12, "start", MUTED)
    s.save("ch15/fig15-6-i2c-register-read.svg")


# ---------------------------------------------------------------------------
# 16장 SPI
# ---------------------------------------------------------------------------

def fig_16_1():
    s = Svg(760, 260)
    s.text(380, 30, "그림 16-1  SPI 연결: 선 네 개, 방향이 정해져 있다", 16, weight="bold")
    s.box(70, 60, 200, 170, "마스터", "STM32", BLUE_BG, BLUE)
    s.box(490, 60, 200, 170, "슬레이브", "SPI 플래시", ORANGE_BG, ORANGE)
    wires = [("SCLK", "SCLK", 90, True, "클럭 (마스터가 만듦)"),
             ("MOSI", "SI", 125, True, "Master Out Slave In"),
             ("MISO", "SO", 160, False, "Master In Slave Out"),
             ("CS", "CS", 195, True, "칩 선택 (Low = 선택)")]
    for l, r, y, right, note in wires:
        s.text(255, y + 5, l, 13, "end", weight="bold")
        s.text(505, y + 5, r, 13, "start", weight="bold")
        if right:
            s.line(272, y, 486, y, BLUE, 2.5, arrow=True)
        else:
            s.line(488, y, 274, y, ORANGE, 2.5, arrow=True)
        s.text(380, y - 6, note, 11, color=MUTED)
    s.text(255, 222, "GND도 공유", 11, "end", MUTED)
    s.save("ch16/fig16-1-spi-wiring.svg")


def fig_16_2():
    s = Svg(820, 280)
    s.text(410, 30, "그림 16-2  SPI는 두 시프트 레지스터를 고리로 이은 것이다", 16, weight="bold")
    def reg(x, title, bits, fill, st):
        s.text(x + 120, 72, title, 14, weight="bold", color=st)
        for i, b in enumerate(bits):
            s.rect(x + i * 30, 85, 30, 40, fill, st, 2)
            s.text(x + i * 30 + 15, 111, b, 14, weight="bold")
    reg(60, "마스터 시프트 레지스터", "10110010", BLUE_BG, BLUE)
    reg(520, "슬레이브 시프트 레지스터", "01011100", ORANGE_BG, ORANGE)
    s.path("M302,105 L380,105", BLUE, 2.5)
    s.line(380, 105, 516, 105, BLUE, 2.5, arrow=True)
    s.text(410, 96, "MOSI", 13, color=BLUE, weight="bold")
    s.path("M762,105 L790,105 L790,190 L30,190 L30,105 L56,105", ORANGE, 2.5)
    s.line(44, 105, 56, 105, ORANGE, 2.5, arrow=True)
    s.text(410, 182, "MISO", 13, color=ORANGE, weight="bold")
    s.text(410, 225, "클럭 한 번에 한 비트씩 양쪽이 동시에 밀려난다. 8클럭 뒤면 두 레지스터의 내용이 서로 바뀐다.", 13)
    s.text(410, 250, "그래서 SPI는 항상 전이중이다. 읽기만 하고 싶어도 아무 값(보통 0xFF)이라도 보내야 한다.", 12, color=MUTED)
    s.save("ch16/fig16-2-spi-shift-ring.svg")


def fig_16_3():
    t = Timing(10, 58, 150, 70, "그림 16-3  SPI 모드 0~3: 클럭의 유휴 레벨(CPOL)과 샘플링 에지(CPHA)")
    t.signal("CS", [(1, "h", None, None), (8.5, "l", None, None), (0.5, "h", None, None)])
    t.signal("SCLK", [(1.5, "l", None, None)] + [(0.5, "h" if k % 2 == 0 else "l", None, None) for k in range(16)] + [(0.5, "l", None, None)], "CPOL = 0")
    t.signal("SCLK", [(1.5, "h", None, None)] + [(0.5, "l" if k % 2 == 0 else "h", None, None) for k in range(16)] + [(0.5, "h", None, None)], "CPOL = 1")
    d0 = [(1, "z", None, None)] + [(1, "d", f"b{7 - i}", BLUE_BG) for i in range(8)] + [(1, "z", None, None)]
    t.signal("데이터", d0, "CPHA = 0")
    d1 = [(1.5, "z", None, None)] + [(1, "d", f"b{7 - i}", ORANGE_BG) for i in range(8)] + [(0.5, "z", None, None)]
    t.signal("데이터", d1, "CPHA = 1")
    t.note("sample", row=3, ts=[1.5 + i for i in range(8)], color=BLUE)
    t.note("sample", row=4, ts=[2.0 + i for i in range(8)], color=ORANGE)
    t.note("text", t=5, y=t.top + 5 * t.ROW + 10, label="▲ 샘플링 시점.  CPHA=0: 첫 번째 에지에서 읽음  ·  CPHA=1: 두 번째 에지에서 읽음", size=13)
    t.note("text", t=5, y=t.top + 5 * t.ROW + 34, label="모드 0 = (0,0)   모드 1 = (0,1)   모드 2 = (1,0)   모드 3 = (1,1)", size=13)
    t.render("ch16/fig16-3-spi-modes.svg", 50)


def fig_16_5():
    s = Svg(880, 330)
    s.text(440, 30, "그림 16-5  슬레이브가 여럿일 때: 개별 CS 방식과 데이지 체인 방식", 16, weight="bold")
    # 왼쪽: 개별 CS
    s.text(210, 56, "(가) 슬레이브마다 CS 하나씩", 14, weight="bold", color=BLUE)
    s.box(40, 90, 110, 190, "마스터", None, BLUE_BG, BLUE)
    s.path("M95,88 L95,74 L420,74 L420,265", MUTED, 2.5)
    s.text(214, 90, "SCLK · MOSI · MISO (모두 공유)", 11, color=MUTED)
    for i in range(3):
        y = 85 + i * 75
        s.box(280, y, 110, 60, f"슬레이브 {i + 1}", None, "#ffffff", LINE, 13)
        s.line(152, y + 30, 276, y + 30, ORANGE, 2, arrow=True)
        s.text(214, y + 24, f"CS{i + 1}", 11, color=ORANGE, weight="bold")
        s.line(420, y + 30, 394, y + 30, MUTED, 2.5, arrow=True)
    s.text(210, 312, "슬레이브가 늘면 CS 핀도 늘어난다", 12, color=MUTED)
    # 오른쪽: 데이지 체인
    s.text(660, 56, "(나) 데이지 체인", 14, weight="bold", color=ORANGE)
    s.box(470, 90, 100, 190, "마스터", None, BLUE_BG, BLUE)
    for i in range(3):
        y = 85 + i * 75
        s.box(700, y, 120, 60, f"슬레이브 {i + 1}", None, "#ffffff", LINE, 13)
    s.line(572, 115, 696, 115, BLUE, 2, arrow=True)
    s.text(634, 109, "MOSI", 11, color=BLUE, weight="bold")
    s.path("M822,125 L846,125 L846,175 L680,175 L680,190 L696,190", BLUE, 2)
    s.path("M822,200 L846,200 L846,250 L680,250 L680,265 L696,265", BLUE, 2)
    s.path("M760,307 L600,307 L600,250 L574,250", ORANGE, 2)
    s.path("M822,280 L846,280 L846,307 L760,307", ORANGE, 2)
    s.text(620, 300, "MISO", 11, color=ORANGE, weight="bold")
    s.text(640, 325, "앞 장치의 출력이 다음 장치의 입력. CS 하나로 길게 밀어 넣는다 (LED 드라이버 등)", 11, color=MUTED)
    s.save("ch16/fig16-5-spi-multi-slave.svg")


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("fig_") and callable(fn):
            fn()
