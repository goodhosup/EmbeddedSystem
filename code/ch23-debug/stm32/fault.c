/*
 * 23장: HardFault 처리기 — 폴트가 나면 원인과 위치를 UART로 출력한다
 *
 * 예외 진입 시 하드웨어가 쌓은 스택 프레임(그림 23-2)에서 PC와 LR을 꺼내고,
 * 시스템 제어 블록의 CFSR, HFSR, MMFAR, BFAR을 읽는다 (PM0214 4.4절).
 * 폴트 상황에서는 printf(힙, 버퍼)를 믿을 수 없으므로 폴링 송신만 쓴다.
 */
#include <stdint.h>

#include "stm32f411_regs.h"
#include "uart2.h"

static void put_str(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart2_putc('\r');
        }
        uart2_putc(*s++);
    }
}

static void put_hex(const char *name, uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    put_str(name);
    put_str("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart2_putc(hex[(v >> i) & 0xFu]);
    }
    put_str("\n");
}

static void explain(uint32_t cfsr, uint32_t hfsr)
{
    if (cfsr & (1u << 25)) put_str("  DIVBYZERO: 0으로 나눔\n");
    if (cfsr & (1u << 24)) put_str("  UNALIGNED: 정렬되지 않은 접근\n");
    if (cfsr & (1u << 19)) put_str("  NOCP: FPU를 켜지 않고 부동소수점 명령\n");
    if (cfsr & (1u << 18)) put_str("  INVPC: 잘못된 예외 복귀\n");
    if (cfsr & (1u << 17)) put_str("  INVSTATE: Thumb 비트가 0인 주소로 점프\n");
    if (cfsr & (1u << 16)) put_str("  UNDEFINSTR: 정의되지 않은 명령\n");
    if (cfsr & (1u << 15)) put_str("  BFARVALID: BFAR이 문제 주소\n");
    if (cfsr & (1u << 12)) put_str("  STKERR: 예외 진입 중 스택 오류 (스택 넘침?)\n");
    if (cfsr & (1u << 10)) put_str("  IMPRECISERR: 부정확한 버스 오류 (PC는 근처일 뿐)\n");
    if (cfsr & (1u << 9))  put_str("  PRECISERR: 정확한 버스 오류 (PC의 명령이 원인)\n");
    if (cfsr & (1u << 8))  put_str("  IBUSERR: 명령 가져오기 버스 오류\n");
    if (cfsr & (1u << 7))  put_str("  MMARVALID: MMFAR이 문제 주소\n");
    if (cfsr & (1u << 1))  put_str("  DACCVIOL: MPU 데이터 접근 위반\n");
    if (cfsr & (1u << 0))  put_str("  IACCVIOL: MPU 실행 위반\n");
    if (hfsr & (1u << 30)) put_str("  (FORCED: 위 폴트가 HardFault로 올라옴)\n");
    if (hfsr & (1u << 1))  put_str("  VECTTBL: 벡터 테이블 읽기 실패\n");
}

/* C에서 불린다. sf = 예외 스택 프레임 [R0, R1, R2, R3, R12, LR, PC, xPSR] */
void hardfault_report(uint32_t *sf)
{
    uint32_t cfsr = SCB_CFSR;
    uint32_t hfsr = SCB_HFSR;

    put_str("\n*** HardFault ***\n");
    put_hex("PC   = ", sf[6]);
    put_hex("LR   = ", sf[5]);
    put_hex("xPSR = ", sf[7]);
    put_hex("R0   = ", sf[0]);
    put_hex("CFSR = ", cfsr);
    put_hex("HFSR = ", hfsr);
    if (cfsr & (1u << 15)) {
        put_hex("BFAR = ", SCB_BFAR);
    }
    if (cfsr & (1u << 7)) {
        put_hex("MMFAR= ", SCB_MMFAR);
    }
    explain(cfsr, hfsr);
    put_str("addr2line -e build/ch23-debug.elf <PC>  로 소스 줄을 찾으세요\n");

    __asm volatile ("bkpt #0");              /* 디버거가 붙어 있으면 여기서 멈춘다 */
    for (;;) {
    }
}

/*
 * 진짜 처리기: 어셈블리 몇 줄로 "폴트가 난 코드가 쓰던 스택"을 골라 C 함수에 넘긴다.
 * EXC_RETURN(LR)의 비트 2가 0이면 MSP, 1이면 PSP (그림 23-2).
 */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "tst   lr, #4          \n"
        "ite   eq              \n"
        "mrseq r0, msp         \n"
        "mrsne r0, psp         \n"
        "b     hardfault_report\n"
    );
}
