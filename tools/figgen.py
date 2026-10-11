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
