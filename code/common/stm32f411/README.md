# STM32F411 공통 파일

이 책의 STM32 예제가 함께 쓰는 파일입니다. 제조사의 HAL이나 CMSIS 패키지 없이 빌드할 수 있도록 최소한으로 작성했습니다.

| 파일 | 내용 | 설명하는 장 |
| --- | --- | --- |
| `stm32f411.ld` | 링커 스크립트 (Flash 512 KB, SRAM 128 KB) | 21장 |
| `startup.c` | 벡터 테이블, 리셋 처리기 (`.data` 복사, `.bss` 초기화) | 7장, 21장 |
| `stm32f411_regs.h` | 예제에서 쓰는 레지스터 정의 (필요한 것만) | 3장, 4장, 6장 |
| `stm32.mk` | 공통 Makefile 규칙 (`USE_NEWLIB=1`이면 newlib-nano 링크) | 5장, 14장, 22장 |
| `syscalls.c` | newlib용 최소 시스템 호출 (`_write`, `_sbrk` 등) | 14장 |
| `uart2.c`, `uart2.h` | USART2 콘솔 모듈 (PA2/PA3, 수신 링 버퍼, `__io_putchar`) | 14장 이후 |

5장 이후에는 같은 일을 ST의 CMSIS 디바이스 헤더(`stm32f4xx.h`)로 하는 방법도 함께 보여 줍니다.
