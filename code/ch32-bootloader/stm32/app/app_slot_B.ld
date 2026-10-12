/* 32장: 슬롯 B용 앱. 슬롯 시작의 512 B는 이미지 머리말 자리이므로 그 뒤부터 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08040200, LENGTH = 128K - 0x200
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
INCLUDE sections.ld
