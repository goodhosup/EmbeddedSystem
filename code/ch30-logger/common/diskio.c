/*
 * 30장: FatFs 저수준 입출력 (diskio) → sd_spi.c
 * FatFs는 이 다섯 함수만으로 저장 장치를 다룬다. 드라이브 번호 0 = SD 카드.
 */
#include "ff.h"
#include "diskio.h"
#include "sd_spi.h"

static DSTATUS stat = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    return (pdrv == 0) ? stat : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    stat = (sd_init() == SD_OK) ? 0 : STA_NOINIT;
    return stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    for (UINT i = 0; i < count; i++) {
        if (sd_read_block((uint32_t)sector + i, buff + i * SD_BLOCK) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    for (UINT i = 0; i < count; i++) {
        if (sd_write_block((uint32_t)sector + i, buff + i * SD_BLOCK) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    switch (cmd) {
    case CTRL_SYNC:
        return (sd_sync() == SD_OK) ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT: {
        uint32_t n;
        if (sd_sector_count(&n) != SD_OK) return RES_ERROR;
        *(LBA_t *)buff = n;
        return RES_OK;
    }
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;          /* 지울 블록 크기를 모름: 1 섹터 */
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
