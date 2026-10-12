/* 40장: 보안 부트로더는 Mbed TLS 때문에 32 KB에 들어가지 않을 수 있다.
 * 섹터 0~3(64 KB)을 쓰려면 32장의 부트 상태 섹터(2, 3)를 섹터 4로 옮겨야 한다(layout.h). [확인 필요: 실제 크기]
 * 먼저 32 KB(boot.ld)로 빌드해 보고, 넘치면 이 배치로 바꾼다. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
INCLUDE sections.ld
