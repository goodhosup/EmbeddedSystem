/*
 * W25Q 계열 SPI NOR 플래시 드라이버
 *
 * 명령은 Winbond W25Q64JV 데이터시트의 명령 표를 따른다(같은 계열은 대부분 공통).
 *   0x9F JEDEC ID          0x03 Read Data (24비트 주소)
 *   0x06 Write Enable      0x05 Read Status Register-1 (bit0 BUSY, bit1 WEL)
 *   0x02 Page Program      0x20 Sector Erase (4 KB)
 *
 * NOR 플래시의 규칙
 *   - 지우기는 섹터(4 KB) 단위로 모든 비트를 1(0xFF)로 만든다.
 *   - 쓰기(프로그램)는 1 → 0만 할 수 있고, 한 번에 한 페이지(256바이트) 안에서만 된다.
 *     페이지 끝을 넘으면 같은 페이지의 처음으로 돌아가 덮어쓰므로 나눠서 보내야 한다.
 *   - 쓰기와 지우기 전에는 매번 Write Enable이 필요하고, 끝날 때까지 BUSY = 1이다.
 */
#include "w25q.h"
#include "spi.h"

#define CMD_JEDEC_ID     0x9Fu
#define CMD_READ         0x03u
#define CMD_WRITE_ENABLE 0x06u
#define CMD_READ_SR1     0x05u
#define CMD_PAGE_PROGRAM 0x02u
#define CMD_SECTOR_ERASE 0x20u

#define SR1_BUSY 0x01u

extern uint32_t board_millis(void);

static void send_addr(uint32_t addr)
{
    spi_xfer((uint8_t)(addr >> 16));
    spi_xfer((uint8_t)(addr >> 8));
    spi_xfer((uint8_t)addr);
}

static uint8_t read_sr1(void)
{
    spi_cs_low();
    spi_xfer(CMD_READ_SR1);
    uint8_t sr = spi_xfer(0xFF);
    spi_cs_high();
    return sr;
}

static int wait_ready(uint32_t timeout_ms)
{
    uint32_t start = board_millis();
    while (read_sr1() & SR1_BUSY) {
        if (board_millis() - start > timeout_ms) {
            return W25Q_ERR_TIMEOUT;
        }
    }
    return W25Q_OK;
}

static void write_enable(void)
{
    spi_cs_low();
    spi_xfer(CMD_WRITE_ENABLE);
    spi_cs_high();
}

void w25q_read_jedec_id(uint8_t id[3])
{
    spi_cs_low();
    spi_xfer(CMD_JEDEC_ID);
    for (int i = 0; i < 3; i++) {
        id[i] = spi_xfer(0xFF);         /* 읽기만 할 때도 무언가 보내야 한다 (그림 16-2) */
    }
    spi_cs_high();
}

void w25q_read(uint32_t addr, uint8_t *buf, size_t n)
{
    spi_cs_low();
    spi_xfer(CMD_READ);
    send_addr(addr);
    for (size_t i = 0; i < n; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    spi_cs_high();
}

int w25q_sector_erase(uint32_t addr)
{
    write_enable();
    spi_cs_low();
    spi_xfer(CMD_SECTOR_ERASE);
    send_addr(addr);
    spi_cs_high();                      /* CS를 올리는 순간 지우기가 시작된다 */
    return wait_ready(500);             /* 4 KB 지우기: 수십~수백 ms (데이터시트 tSE) */
}

static int page_program(uint32_t addr, const uint8_t *data, size_t n)
{
    write_enable();
    spi_cs_low();
    spi_xfer(CMD_PAGE_PROGRAM);
    send_addr(addr);
    for (size_t i = 0; i < n; i++) {
        spi_xfer(data[i]);
    }
    spi_cs_high();
    return wait_ready(10);              /* 페이지 쓰기: 수 ms 이하 (데이터시트 tPP) */
}

int w25q_write(uint32_t addr, const uint8_t *data, size_t n)
{
    while (n > 0) {
        size_t room = W25Q_PAGE_SIZE - (addr % W25Q_PAGE_SIZE);   /* 이 페이지에 남은 칸 */
        size_t chunk = n < room ? n : room;
        int r = page_program(addr, data, chunk);
        if (r != W25Q_OK) {
            return r;
        }
        addr += (uint32_t)chunk;
        data += chunk;
        n -= chunk;
    }
    return W25Q_OK;
}
