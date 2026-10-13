/* 40장: 보안 부트로더 (섹터 0~1, 32 KB).
 * Mbed TLS를 넣고 -Os로 빌드하면 text+data 약 22 KB로 들어간다 (arm-none-eabi-gcc 14.2.1).
 * 기능을 더해 32 KB를 넘으면 섹터 0~3(64 KB)을 쓰고, 32장의 부트 상태 섹터(2, 3)를
 * 섹터 4로 옮겨야 한다(layout.h). 섹터 4의 공장 데이터(36장)도 함께 옮긴다. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
INCLUDE sections.ld
