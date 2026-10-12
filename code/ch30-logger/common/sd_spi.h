/*
 * 30장: SD 카드 SPI 모드 드라이버
 *   16장의 spi.h(spi_init, spi_xfer, spi_cs_low/high)만 쓴다.
 *   SD Physical Layer Simplified Specification의 SPI 모드 명령: CMD0, CMD8, ACMD41, CMD58, CMD16, CMD17, CMD24, CMD9
 */
#ifndef SD_SPI_H
#define SD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK 512u

typedef enum { SD_NONE = 0, SD_V1_SDSC, SD_V2_SDSC, SD_V2_SDHC } sd_type_t;

typedef enum {
    SD_OK = 0,
    SD_ERR_NO_CARD = -1,     /* CMD0에 대답 없음 */
    SD_ERR_INIT = -2,        /* 초기화 시간 초과, 지원하지 않는 카드 */
    SD_ERR_CMD = -3,         /* 명령 응답 오류 */
    SD_ERR_TIMEOUT = -4,     /* 데이터 토큰이나 busy 해제를 기다리다 시간 초과 */
    SD_ERR_WRITE = -5,       /* 카드가 데이터를 거부함 */
} sd_err_t;

/* 플랫폼이 제공: 밀리초 시각 (시간 제한용) */
uint32_t sd_millis(void);

sd_err_t  sd_init(void);
sd_type_t sd_type(void);
sd_err_t  sd_read_block(uint32_t lba, uint8_t *buf);
sd_err_t  sd_write_block(uint32_t lba, const uint8_t *buf);
sd_err_t  sd_sector_count(uint32_t *count);   /* CSD에서 계산 */
sd_err_t  sd_sync(void);                      /* 쓰기가 끝날 때까지 기다림 */

#endif
