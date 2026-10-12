# 36장 실습: CI/CD와 양산

```
ch36-cicd/
├── ci/
│   ├── run_host_tests.sh   이 책의 모든 PC 테스트를 한 번에 (로컬과 CI에서 같게)
│   └── firmware.yml        GitHub Actions 예: PC 테스트 → 정적 분석 → ARM 빌드 → HIL → 릴리스
│                           (.github/workflows/로 복사해야 실행된다)
├── crashlog/               현장 크래시 기록 (Flash 섹터 7에 32 B씩 덧붙이기)
├── test/                   crashlog PC 테스트 (32장 fake_flash 사용)
└── factory/
    ├── stm32/              공장 시험 펌웨어 (ID, RAM, I2C, GPIO, SERIAL)
    └── factory_station.py  시험 PC 프로그램: 시험 → 일련번호 기록 → 다시 읽기 → CSV
```

```bash
sh ci/run_host_tests.sh                     # 저장소 어디서나
cd test && make
cd ../factory/stm32 && make flash
python3 ../factory_station.py --port /dev/ttyACM0 --log results.csv --provision
```

[확인 필요: 공장 시험 펌웨어의 보드 실행, GitHub Actions에서 firmware.yml 실행]
