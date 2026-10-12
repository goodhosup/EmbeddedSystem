# 35장 실습: 기능 안전의 기본 도구

```
ch35-safety/
├── common/
│   ├── supervisor.c/.h   태스크 감시자: 모든 태스크가 기한 안에 체크인했을 때만 워치독 갱신
│   ├── march.c/.h        RAM 시험 March C- (메모리 접근을 함수로 받아 고장 주입 가능)
│   └── checked.h         넘침을 확인하는 산술 (__builtin_*_overflow)
├── test/
│   ├── test_safety.c     감시자, 고착·결합 고장 주입, 산술 시험
│   └── bad_examples.c    정적 분석이 찾는 버그 4개 (make analyze)
└── stm32/                리셋 원인 출력, 시작 시 RAM 시험, IWDG + 감시자 (h: 태스크 하나 멈춤, w: 전체 멈춤)
```

```bash
cd test && make            # PC 테스트
make analyze               # GCC -fanalyzer (cppcheck가 있으면 함께)
cd ../stm32 && make flash
```

[확인 필요: 보드 실행 결과]
