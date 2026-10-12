/* 32장: 부트로더는 Flash 맨 앞 32 KB (섹터 0~1) */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
INCLUDE sections.ld
