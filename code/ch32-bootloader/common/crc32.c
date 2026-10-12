#include "crc32.h"

/* 4비트씩 처리하는 표(16개, 64 B): 비트 단위보다 약 4배 빠르고 256개 표보다 Flash를 적게 쓴다 */
static const uint32_t tbl[16] = {
    0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
    0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu,
};

uint32_t crc32_update(uint32_t crc, const void *buf, size_t n)
{
    const uint8_t *p = (const uint8_t *)buf;
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        crc = (crc >> 4) ^ tbl[crc & 0x0Fu];
        crc = (crc >> 4) ^ tbl[crc & 0x0Fu];
    }
    return ~crc;
}
