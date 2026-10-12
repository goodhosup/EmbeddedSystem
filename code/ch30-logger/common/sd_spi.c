/*
 * 30장: SD 카드 SPI 모드 드라이버
 *
 * 초기화 순서 (SPI 모드):
 *   1. CS High 상태로 74클럭 이상 (0xFF 10바이트)
 *   2. CMD0  → R1 = 0x01 (idle)              SPI 모드로 들어감
 *   3. CMD8  → R7, 인자 0x1AA를 되돌려줌      v2 카드 확인 (illegal command면 v1)
 *   4. ACMD41(HCS) 반복 → R1 = 0x00           초기화 끝 (최대 1초)
 *   5. CMD58 → OCR의 CCS 비트                 1이면 SDHC/SDXC: 블록 번호로 주소 지정
 *   6. (SDSC만) CMD16(512)                    블록 길이 512
 * 초기화는 100~400 kHz에서, 그 뒤로는 빠르게 (spi_init을 다시 부른다).
 */
#include "sd_spi.h"
#include "spi.h"

#define CMD0   0u      /* GO_IDLE_STATE */
#define CMD8   8u      /* SEND_IF_COND */
#define CMD9   9u      /* SEND_CSD */
#define CMD16  16u     /* SET_BLOCKLEN */
#define CMD17  17u     /* READ_SINGLE_BLOCK */
#define CMD24  24u     /* WRITE_BLOCK */
#define CMD55  55u     /* APP_CMD: 다음 명령은 ACMD */
#define CMD58  58u     /* READ_OCR */
#define ACMD41 41u     /* SD_SEND_OP_COND */

#define R1_IDLE        0x01u
#define R1_ILLEGAL_CMD 0x04u
#define TOKEN_START    0xFEu

#define INIT_HZ  250000u        /* 400 kHz 이하 */
#define FAST_HZ  8000000u

static sd_type_t card;

static bool wait_ready(uint32_t timeout_ms)
{
    uint32_t t0 = sd_millis();
    do {
        if (spi_xfer(0xFF) == 0xFF) {        /* busy 동안 카드는 MISO를 Low로 붙잡는다 */
            return true;
        }
    } while (sd_millis() - t0 < timeout_ms);
    return false;
}

static void deselect(void)
{
    spi_cs_high();
    (void)spi_xfer(0xFF);                     /* CS를 올린 뒤 클럭 하나: 카드가 MISO를 놓게 */
}

static bool select_card(void)
{
    spi_cs_low();
    (void)spi_xfer(0xFF);
    if (wait_ready(500)) {
        return true;
    }
    deselect();
    return false;
}

/* 명령을 보내고 R1을 돌려준다 (0x80 비트가 서 있으면 실패). CS는 Low로 남겨 둔다 */
static uint8_t send_cmd(uint8_t cmd, uint32_t arg)
{
    if (cmd & 0x80u) {                        /* ACMD = CMD55 + CMD */
        cmd &= 0x7Fu;
        uint8_t r = send_cmd(CMD55, 0);
        if (r > 1u) {
            return r;
        }
    }
    deselect();
    if (!select_card()) {
        return 0xFF;
    }
    (void)spi_xfer((uint8_t)(0x40u | cmd));
    (void)spi_xfer((uint8_t)(arg >> 24));
    (void)spi_xfer((uint8_t)(arg >> 16));
    (void)spi_xfer((uint8_t)(arg >> 8));
    (void)spi_xfer((uint8_t)arg);
    uint8_t crc = 0x01u;                      /* SPI 모드에서는 CMD0, CMD8만 CRC를 검사한다 */
    if (cmd == CMD0) crc = 0x95u;
    if (cmd == CMD8) crc = 0x87u;
    (void)spi_xfer(crc);

    uint8_t r;
    for (int i = 0; i < 10; i++) {            /* 응답은 1~8바이트 뒤에 온다 */
        r = spi_xfer(0xFF);
        if ((r & 0x80u) == 0) {
            break;
        }
    }
    return r;
}

sd_err_t sd_init(void)
{
    card = SD_NONE;
    spi_init(INIT_HZ);
    spi_cs_high();
    for (int i = 0; i < 10; i++) {            /* 80클럭 */
        (void)spi_xfer(0xFF);
    }

    if (send_cmd(CMD0, 0) != R1_IDLE) {
        deselect();
        return SD_ERR_NO_CARD;
    }

    sd_type_t type;
    uint32_t acmd41_arg;
    uint8_t r = send_cmd(CMD8, 0x1AAu);
    if (r == R1_IDLE) {
        uint8_t r7[4];
        for (int i = 0; i < 4; i++) {
            r7[i] = spi_xfer(0xFF);
        }
        if ((r7[2] & 0x0Fu) != 0x01u || r7[3] != 0xAAu) {    /* 전압 범위(하위 4비트) 2.7~3.6 V와 확인 패턴 */
            deselect();
            return SD_ERR_INIT;
        }
        type = SD_V2_SDSC;
        acmd41_arg = 1u << 30;                     /* HCS: 고용량 카드를 지원한다고 알림 */
    } else if (r & R1_ILLEGAL_CMD) {
        type = SD_V1_SDSC;                         /* 오래된 v1 카드 */
        acmd41_arg = 0;
    } else {
        deselect();
        return SD_ERR_INIT;
    }

    uint32_t t0 = sd_millis();
    do {
        r = send_cmd(0x80u | ACMD41, acmd41_arg);
    } while (r == R1_IDLE && sd_millis() - t0 < 1000u);
    if (r != 0) {
        deselect();
        return SD_ERR_INIT;
    }

    if (type == SD_V2_SDSC) {
        if (send_cmd(CMD58, 0) != 0) {
            deselect();
            return SD_ERR_CMD;
        }
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) {
            ocr[i] = spi_xfer(0xFF);
        }
        if (ocr[0] & 0x40u) {                      /* CCS 비트 (OCR 비트 30) */
            type = SD_V2_SDHC;
        }
    }
    if (type != SD_V2_SDHC && send_cmd(CMD16, SD_BLOCK) != 0) {
        deselect();
        return SD_ERR_CMD;
    }
    deselect();

    card = type;
    spi_init(FAST_HZ);
    return SD_OK;
}

sd_type_t sd_type(void)
{
    return card;
}

static uint32_t to_addr(uint32_t lba)
{
    return (card == SD_V2_SDHC) ? lba : lba * SD_BLOCK;   /* SDSC는 바이트 주소 */
}

static sd_err_t read_data(uint8_t *buf, uint32_t n)
{
    uint32_t t0 = sd_millis();
    uint8_t tok;
    do {
        tok = spi_xfer(0xFF);
    } while (tok == 0xFF && sd_millis() - t0 < 200u);
    if (tok != TOKEN_START) {
        return SD_ERR_TIMEOUT;
    }
    for (uint32_t i = 0; i < n; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    (void)spi_xfer(0xFF);                      /* CRC 2바이트 (SPI 모드에서는 검사하지 않음) */
    (void)spi_xfer(0xFF);
    return SD_OK;
}

sd_err_t sd_read_block(uint32_t lba, uint8_t *buf)
{
    if (card == SD_NONE) {
        return SD_ERR_NO_CARD;
    }
    sd_err_t e = SD_ERR_CMD;
    if (send_cmd(CMD17, to_addr(lba)) == 0) {
        e = read_data(buf, SD_BLOCK);
    }
    deselect();
    return e;
}

sd_err_t sd_write_block(uint32_t lba, const uint8_t *buf)
{
    if (card == SD_NONE) {
        return SD_ERR_NO_CARD;
    }
    sd_err_t e = SD_ERR_CMD;
    if (send_cmd(CMD24, to_addr(lba)) == 0) {
        (void)spi_xfer(0xFF);
        (void)spi_xfer(TOKEN_START);
        for (uint32_t i = 0; i < SD_BLOCK; i++) {
            (void)spi_xfer(buf[i]);
        }
        (void)spi_xfer(0xFF);                  /* 가짜 CRC */
        (void)spi_xfer(0xFF);
        uint8_t resp = spi_xfer(0xFF);         /* 데이터 응답: xxx0 0101 = 받아들임 */
        if ((resp & 0x1Fu) == 0x05u) {
            e = wait_ready(500) ? SD_OK : SD_ERR_TIMEOUT;   /* 카드가 Flash에 쓰는 동안 busy */
        } else {
            e = SD_ERR_WRITE;
        }
    }
    deselect();
    return e;
}

sd_err_t sd_sector_count(uint32_t *count)
{
    uint8_t csd[16];
    sd_err_t e = SD_ERR_CMD;
    if (send_cmd(CMD9, 0) == 0 && read_data(csd, 16) == SD_OK) {
        if ((csd[0] >> 6) == 1u) {             /* CSD 버전 2.0 (SDHC/SDXC) */
            uint32_t c_size = ((uint32_t)(csd[7] & 0x3Fu) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
            *count = (c_size + 1u) * 1024u;    /* (C_SIZE+1) × 512 KB / 512 B */
        } else {                               /* CSD 버전 1.0 */
            uint32_t read_bl_len = csd[5] & 0x0Fu;
            uint32_t c_size = ((uint32_t)(csd[6] & 0x03u) << 10) | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
            uint32_t c_size_mult = ((csd[9] & 0x03u) << 1) | (csd[10] >> 7);
            *count = (c_size + 1u) << (c_size_mult + 2u + read_bl_len - 9u);
        }
        e = SD_OK;
    }
    deselect();
    return e;
}

sd_err_t sd_sync(void)
{
    sd_err_t e = SD_OK;
    if (select_card() == false) {
        e = SD_ERR_TIMEOUT;
    }
    deselect();
    return e;
}
