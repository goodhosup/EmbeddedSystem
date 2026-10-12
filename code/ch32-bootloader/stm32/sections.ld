/*
 * 32장: 부트로더와 앱이 함께 쓰는 섹션 배치
 * 21장의 code/common/stm32f411/stm32f411.ld에서 SECTIONS 부분만 그대로 옮겼다.
 * MEMORY(어디에 놓을지)는 boot.ld, app_slot_A.ld, app_slot_B.ld가 각자 정하고 이 파일을 INCLUDE 한다.
 */

ENTRY(Reset_Handler)

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

    /DISCARD/ :
    {
        *(.ARM.exidx*)
    }
}
