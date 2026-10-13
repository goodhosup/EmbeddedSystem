# 31장 실습: 테스트 가능한 펌웨어

```
ch31-testing/
├── get_unity.sh            Unity v2.6.1을 lib/unity에 받는다
├── src/thermostat.c/.h     TDD로 만든 히터 온도 조절기
├── test/                   PC 테스트 (Unity)
│   ├── test_thermostat.c   스텁(fake_read)과 스파이(spy_heater)
│   ├── test_bmp280.c       15장 BMP280 드라이버를 I2C 목으로
│   ├── mock_i2c.c/.h       손으로 쓴 CMock 스타일 목
│   └── main_host.c
├── stm32/                  같은 테스트를 보드에서 실행 (결과는 USART2)
└── hil/run_on_target.py    보드 테스트 결과를 시리얼로 읽어 판정 (pyserial)
```

```bash
./get_unity.sh
cd test && make             # PC: 15 Tests 0 Failures
make coverage               # gcov 줄 커버리지
cd ../stm32 && make flash   # 보드
python3 ../hil/run_on_target.py --port /dev/ttyACM0 --flash
```

빌드 확인: arm-none-eabi-gcc 14.2.1, 경고 0 (Unity v2.6.1 포함, text 10,404 B). [확인 필요: 보드 실행 결과와 HIL 스크립트 동작]
