/* Winbond W25Q 계열 SPI NOR 플래시 드라이버 (16장) */
#ifndef W25Q_H
#define W25Q_H

#include <stddef.h>
#include <stdint.h>

#define W25Q_PAGE_SIZE    256u
#define W25Q_SECTOR_SIZE  4096u

#define W25Q_OK           0
#define W25Q_ERR_TIMEOUT  (-1)
#define W25Q_ERR_ARG      (-2)

void w25q_read_jedec_id(uint8_t id[3]);                          /* 제조사, 메모리 종류, 용량 */
void w25q_read(uint32_t addr, uint8_t *buf, size_t n);
int  w25q_sector_erase(uint32_t addr);                           /* addr이 속한 4 KB를 0xFF로 */
int  w25q_write(uint32_t addr, const uint8_t *data, size_t n);  /* 페이지 경계를 나눠 쓴다 */

#endif
