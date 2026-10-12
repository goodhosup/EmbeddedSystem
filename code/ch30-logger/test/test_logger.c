/*
 * 30장: 데이터 로거의 PC 테스트
 *   1) logrec: CSV 한 줄 형식
 *   2) sd_spi: 가짜 카드와 초기화, 블록 읽기/쓰기
 *   3) FatFs + logger: 포맷 → 기록 → 다시 읽어 비교, 파일 번호 증가, 카드 뽑힘
 * 실제 SD 카드 없이 SPI 바이트 단위의 프로토콜부터 파일 시스템까지 한 번에 시험한다.
 */
#include <stdio.h>
#include <string.h>

#include "fake_sd.h"
#include "sd_spi.h"
#include "logrec.h"
#include "logger.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_logrec(void)
{
    char b[LOGREC_MAX_LINE];
    logrec_t r = { 12, 3600, 2487, 0 };
    CHECK(logrec_format(b, sizeof b, &r) == 17 && strcmp(b, "12,3600,24.87,0\r\n") == 0);
    r.temp_c100 = -5;
    logrec_format(b, sizeof b, &r);
    CHECK(strcmp(b, "12,3600,-0.05,0\r\n") == 0);         /* -0.05: 부호를 잃지 않는다 */
    r.temp_c100 = -1234;
    logrec_format(b, sizeof b, &r);
    CHECK(strcmp(b, "12,3600,-12.34,0\r\n") == 0);
    r.flags = LOGREC_F_SENSOR_ERR;
    logrec_format(b, sizeof b, &r);
    CHECK(strcmp(b, "12,3600,,1\r\n") == 0);              /* 센서 오류: 값 칸을 비운다 */
    logrec_t big = { 4294967295u, 4294967295u, -2147483647 - 1, 3 };
    CHECK(logrec_format(b, sizeof b, &big) > 0);           /* 가장 긴 줄도 버퍼에 들어간다 */
    CHECK(logrec_format(b, 8, &r) == 0);                   /* 버퍼가 작으면 0 */
}

static void test_sd(void)
{
    fake_sd_insert(true);
    CHECK(sd_init() == SD_OK);
    CHECK(sd_type() == SD_V2_SDHC);
    uint32_t n = 0;
    CHECK(sd_sector_count(&n) == SD_OK && n == FAKE_SD_SECTORS);

    uint8_t w[512], r[512];
    for (int i = 0; i < 512; i++) w[i] = (uint8_t)(i * 7 + 3);
    CHECK(sd_write_block(100, w) == SD_OK);
    memset(r, 0, sizeof r);
    CHECK(sd_read_block(100, r) == SD_OK);
    CHECK(memcmp(w, r, 512) == 0);
    CHECK(memcmp(fake_sd_image() + 100 * 512, w, 512) == 0);   /* 정말 블록 100에 들어갔는가 */

    fake_sd_insert(false);
    CHECK(sd_init() == SD_ERR_NO_CARD);
    fake_sd_insert(true);
}

static void format_card(void)
{
    static FATFS fs;
    static BYTE work[FF_MAX_SS];
    MKFS_PARM opt = { FM_FAT, 1, 0, 0, 0 };
    CHECK(f_mount(&fs, "", 0) == FR_OK);
    CHECK(f_mkfs("", &opt, work, sizeof work) == FR_OK);
    f_mount(NULL, "", 0);
}

static void test_logger(void)
{
    fake_sd_insert(true);
    format_card();

    static logger_t lg;
    CHECK(logger_start(&lg, 10) == FR_OK);
    CHECK(strcmp(lg.name, "LOG00001.CSV") == 0);
    uint32_t w0 = fake_sd_writes();
    for (uint32_t i = 0; i < 25; i++) {
        logrec_t r = { i, i * 10u, 2000 + (int32_t)i, 0 };
        CHECK(logger_write(&lg, &r) == FR_OK);
    }
    CHECK(lg.lines == 25 && lg.unsynced == 5);            /* 10, 20에서 sync */
    CHECK(fake_sd_writes() > w0);
    CHECK(logger_stop(&lg) == FR_OK);

    /* 다시 마운트해서 파일 내용을 처음부터 읽어 본다 */
    static FATFS fs;
    static FIL f;
    static char buf[2048];
    UINT br;
    CHECK(f_mount(&fs, "", 1) == FR_OK);
    CHECK(f_open(&f, "LOG00001.CSV", FA_READ) == FR_OK);
    CHECK(f_read(&f, buf, sizeof buf - 1, &br) == FR_OK);
    buf[br] = '\0';
    f_close(&f);
    f_mount(NULL, "", 0);
    CHECK(strncmp(buf, LOGREC_HEADER, strlen(LOGREC_HEADER)) == 0);
    CHECK(strstr(buf, "0,0,20.00,0\r\n") != NULL);
    CHECK(strstr(buf, "24,240,20.24,0\r\n") != NULL);
    int lines = 0;
    for (char *p = buf; *p; p++) lines += (*p == '\n');
    CHECK(lines == 26);                                     /* 머리줄 + 25 */

    /* 다시 켜면 다음 번호의 파일을 만든다 (지난 기록을 덮어쓰지 않음) */
    fake_sd_reset();
    CHECK(logger_start(&lg, 10) == FR_OK);
    CHECK(strcmp(lg.name, "LOG00002.CSV") == 0);
    logger_stop(&lg);
}

static void test_power_loss(void)
{
    /* sync 전에 전원이 꺼지면? 마지막 sync까지만 남는다 */
    static logger_t lg;
    fake_sd_insert(true);
    format_card();
    CHECK(logger_start(&lg, 10) == FR_OK);
    for (uint32_t i = 0; i < 15; i++) {
        logrec_t r = { i, i, 100, 0 };
        logger_write(&lg, &r);
    }
    /* logger_stop 없이 전원 차단 */
    fake_sd_reset();

    static FATFS fs;
    static FILINFO fi;
    CHECK(f_mount(&fs, "", 1) == FR_OK);
    CHECK(f_stat("LOG00001.CSV", &fi) == FR_OK);
    /* 디렉터리의 파일 크기는 10줄을 sync한 시점의 값 */
    CHECK(fi.fsize == strlen(LOGREC_HEADER) + 10u * strlen("0,0,1.00,0\r\n"));
    f_mount(NULL, "", 0);
    printf("after power loss: %lu bytes kept (header + 10 lines)\n", (unsigned long)fi.fsize);
}

static void test_card_removed(void)
{
    static logger_t lg;
    fake_sd_insert(false);
    CHECK(logger_start(&lg, 10) != FR_OK);
    CHECK(lg.errors == 1 && !lg.open);
    logrec_t r = { 0, 0, 0, 0 };
    CHECK(logger_write(&lg, &r) == FR_NOT_READY);
    fake_sd_insert(true);
}

int main(void)
{
    fake_sd_reset();
    test_logrec();
    test_sd();
    test_logger();
    test_power_loss();
    test_card_removed();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all tests passed", fails);
    return fails != 0;
}
