/*
 * STM32F411RE 링커 스크립트 (이 책의 STM32 예제 공통)
 *
 * 메모리 크기와 시작 주소: RM0383 2.3절 "Memory map",
 * STM32F411xC/E 데이터시트(DS10314) 메모리 매핑 그림.
 * 21장에서 이 파일을 한 줄씩 해부한다.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* 스택은 RAM의 끝에서 아래로 자란다 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 생성자 함수 포인터 목록: C의 __attribute__((constructor)), C++ 전역 객체 (21장) */
    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH

    /* 초기값이 있는 전역 변수: Flash에 저장해 두었다가 시작할 때 RAM으로 복사 */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* 초기값이 없는 전역 변수: 시작할 때 0으로 채움 */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* 힙은 .bss 바로 뒤에서 시작해 스택 쪽으로 자란다 (newlib의 malloc이 _sbrk로 요청) */
    end = _ebss;
    _end = _ebss;

    /* newlib의 setjmp 등이 C++ 예외 해제 루틴(libgcc)을 끌어온다. 해제 표는 쓰지 않으므로
       아래에서 버리고, 그 루틴이 찾는 기호만 빈 표로 정의한다 (31장 Unity의 setjmp) */
    PROVIDE(__exidx_start = .);
    PROVIDE(__exidx_end = .);

    /DISCARD/ :
    {
        *(.ARM.exidx*)
    }
}
