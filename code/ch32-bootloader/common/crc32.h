/* 32장: CRC-32 (IEEE 802.3, zlib과 같은 값). 13장의 CRC를 32비트로 */
#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

/* crc = crc32_update(0, buf, n); 이어서 계산할 때는 앞의 결과를 넘긴다 */
uint32_t crc32_update(uint32_t crc, const void *buf, size_t n);

#endif
