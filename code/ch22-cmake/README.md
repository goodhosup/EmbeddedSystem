# 22장 실습: CMake로 STM32 펌웨어 빌드

```bash
cmake -B build-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
cmake --build build-debug --target flash
```

| 파일 | 내용 |
| --- | --- |
| `cmake/arm-none-eabi.cmake` | 툴체인 파일: 컴파일러, CPU 옵션, 빌드 종류별 최적화 |
| `CMakeLists.txt` | 타깃: `app`(보드 독립 정적 라이브러리), `board`(스타트업 등 오브젝트 라이브러리), `firmware`(.elf), `flash` |
| `src/version.h.in` | git 버전이 들어가는 헤더의 틀 |
| `src/average.c` | 이동 평균 모듈 (31장에서 PC로 시험) |

CMake 3.20 이상과 Ninja가 필요합니다(Make도 됩니다: `-G "Unix Makefiles"`).
