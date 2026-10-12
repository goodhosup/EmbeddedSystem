# 29장 실습: Zephyr

15장의 BMP280(I2C1, PB8/PB9, 주소 0x76)과 보드의 LD2, B1을 Zephyr로 다룹니다.

```bash
# Zephyr 작업 공간이 ~/zephyrproject에 있다고 할 때 (29.2절)
cd ~/zephyrproject
west build -p always -b nucleo_f411re /path/to/embeddedsystem/code/ch29-zephyr
west flash
```

| 파일 | 역할 |
| --- | --- |
| `CMakeLists.txt` | Zephyr를 찾아 앱 소스를 등록 |
| `prj.conf` | Kconfig: 켤 기능 |
| `boards/nucleo_f411re.overlay` | devicetree 오버레이: I2C1에 BMP280 노드 추가 |
| `src/main.c` | 스레드 2개 + main, 메시지 큐, 입력 콜백 |

시리얼 터미널(115200 bps)에 1초마다 온도와 기압이 나오고, B1로 측정을 멈추고 다시 켭니다.

[확인 필요: Zephyr v4.4 기준 west build와 보드 실행 확인 (외부 코드라 검토 환경에서 미빌드)]
