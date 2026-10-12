# 부록 F. 더 공부할 자료

이 책을 마친 독자가 다음에 읽으면 좋은 자료를 주제별로 골랐습니다. 대부분 영어 자료입니다. 판(edition)과 주소는 출간 시점에 다시 확인하세요. [확인 필요: 각 자료의 최신 판과 주소]

## F.1 제조사 문서 (가장 중요한 자료)

어떤 책보다 정확한 자료는 칩 제조사의 문서입니다(부록 A).

- **ST**: STM32F411 데이터시트(DS10314), 레퍼런스 매뉴얼(RM0383), Cortex-M4 프로그래밍 매뉴얼(PM0214), NUCLEO 사용자 설명서(UM1724), 에라타, 애플리케이션 노트. st.com의 제품 페이지에 모여 있습니다.
- **Microchip**: ATmega328P, ATmega128A 데이터시트와 애플리케이션 노트.
- **Espressif**: ESP32 기술 참조 매뉴얼(Technical Reference Manual), ESP-IDF 프로그래밍 가이드(docs.espressif.com).
- **Arm**: Cortex-M4 기술 참조 매뉴얼, Armv7-M 아키텍처 참조 매뉴얼(developer.arm.com). 예외, 메모리 모델, 명령어의 최종 근거입니다.
- **Bosch, Winbond 등 부품 제조사**: 15장, 16장에서 쓴 센서와 Flash의 데이터시트.

## F.2 책

**임베디드 시스템 전반**
- Elecia White, *Making Embedded Systems* (O'Reilly): 설계와 구조, 디버깅을 실무 관점에서 다룹니다. 이 책의 24장, 30장과 잘 어울립니다.
- Jonathan Valvano의 Cortex-M 교재 시리즈: 레지스터 수준에서 주변장치와 실시간 시스템을 다룹니다.

**Cortex-M 아키텍처**
- Joseph Yiu, *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors* (Newnes): 예외, NVIC, MPU, FPU를 가장 자세히 설명합니다. 7장, 21장, 23장, 26장을 더 깊이 이해하고 싶을 때. 같은 저자의 Cortex-M23/M33 책은 TrustZone-M(33장)을 다룹니다.

**시험과 설계**
- James Grenning, *Test-Driven Development for Embedded C* (Pragmatic Bookshelf): 31장의 TDD와 테스트 더블을 깊이 다룹니다.
- Miro Samek, *Practical UML Statecharts in C/C++*: 24장의 상태 머신과 액티브 객체를 체계적으로 다룹니다.

**RTOS와 실시간**
- *Mastering the FreeRTOS Real Time Kernel*: FreeRTOS 공식 안내서로, freertos.org에서 무료로 받을 수 있습니다(26~27장).
- Jane W. S. Liu, *Real-Time Systems*: 28장의 스케줄 가능성 분석의 이론적 배경.

**리눅스**
- Bootlin의 임베디드 리눅스, 커널 드라이버 교육 자료: 슬라이드와 실습 자료가 자유 라이선스로 공개되어 있습니다(bootlin.com). 37~38장 다음 단계로 가장 추천합니다.
- John Madieu, *Linux Device Driver Development* (Packt): 최신 커널 기준의 드라이버 개발.

**보안**
- David Kleidermacher, Mike Kleidermacher, *Embedded Systems Security* (Newnes): 33장의 내용을 제품 관점에서 넓게 다룹니다.

## F.3 온라인 문서와 블로그

- **Interrupt** (Memfault 블로그, interrupt.memfault.com): 펌웨어 디버깅, 크래시 분석(23장, 36장), 빌드, 시험에 관한 깊이 있는 글이 많습니다.
- **Embedded Artistry** (embeddedartistry.com): 임베디드 소프트웨어 설계와 자료 모음.
- **The Embedded Rust Book** (docs.rust-embedded.org/book): 39장의 다음 단계. 같은 사이트에 Discovery 책(입문)과 Embedonomicon(내부 동작)이 있습니다.
- **Zephyr 문서** (docs.zephyrproject.org): 29장의 개념을 자세히 다루며, 보드마다 시작 안내가 있습니다.
- **리눅스 커널 문서** (docs.kernel.org): 드라이버 모델, 디바이스 트리, IIO, GPIO 서브시스템(38장)의 공식 문서.
- **MCUboot 문서** (docs.mcuboot.com): 32장, 33장의 보안 부트와 업데이트.
- **SEI CERT C 코딩 표준** (wiki.sei.cmu.edu): 35장의 CERT C 규칙과 예제.

## F.4 오픈소스 코드 읽기

좋은 펌웨어 코드를 읽는 것은 가장 효과적인 공부입니다. 이 책에서 쓴 프로젝트들은 모두 공개되어 있습니다.

| 프로젝트 | 읽어 볼 곳 | 관련 장 |
| --- | --- | --- |
| FreeRTOS 커널 | `tasks.c`의 스케줄러, `portable/GCC/ARM_CM4F/port.c`의 PendSV | 26장 |
| Zephyr | `drivers/` (드라이버 모델), `boards/` (보드 정의) | 29장 |
| TinyUSB | 디스크립터 처리, 클래스 드라이버 | 18장 |
| FatFs | `ff.c`의 FAT 처리 (주석이 아주 자세함) | 30장 |
| Mbed TLS | `library/ecdsa.c`, `library/sha256.c` | 33장 |
| MCUboot | `boot/bootutil/` (교환, 검증) | 32장 |
| 리눅스 커널 | `drivers/iio/pressure/bmp280-*.c` (38장 드라이버의 완전판) | 38장 |
| embassy | `embassy-stm32` (async HAL) | 39장 |

처음에는 이 책에서 직접 만든 것과 같은 일을 하는 부분(예: FreeRTOS의 PendSV, 리눅스의 BMP280 드라이버)부터 읽으면, 실제 제품 수준의 코드가 무엇을 더 신경 쓰는지 비교하며 배울 수 있습니다.

## F.5 다음 프로젝트 제안

이 책의 실습을 마쳤다면, 직접 정한 프로젝트를 끝까지 만들어 보는 것이 가장 좋은 다음 단계입니다. 30장과 40장처럼 설계 문서부터 시작하세요.

- **배터리로 1년 가는 무선 센서**: 11장의 저전력, 19장의 BLE, 30장의 전류 프로파일. 평균 전류를 µA 단위로 줄이는 과정에서 많이 배웁니다.
- **작은 로봇이나 자세 제어**: 8장의 PWM, IMU 센서(I2C/SPI), 28장의 실시간 분석, 34장의 고정소수점 필터.
- **USB 장치**: 18장의 CDC를 넘어 HID(키보드, 게임패드)나 오디오 장치.
- **자기만의 PCB**: 부록 B의 내용으로 30장의 데이터 로거를 PCB로 만들고, 36장의 공장 시험까지 직접 해 보기.
- **오픈소스 기여**: 쓰는 칩이나 센서의 Zephyr, 리눅스 드라이버가 없다면 만들어 기여하기. 리뷰를 받는 과정 자체가 훌륭한 공부입니다.

임베디드는 손으로 만지며 배우는 분야입니다. 데이터시트를 열고, 회로를 연결하고, 오실로스코프로 확인하고, 시험을 쓰는 그 과정을 계속 반복하는 것이 가장 확실한 길입니다.
