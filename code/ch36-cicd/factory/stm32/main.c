/*
 * 36장: 공장 시험 펌웨어 (NUCLEO-F411RE)
 *
 * 생산 라인의 시험 PC(factory_station.py)가 UART로 한 줄 명령을 보내면 JSON 한 줄로 답한다.
 *   ID            칩 고유 번호(96비트), Flash 크기, 펌웨어 이름
 *   RAM           March C- RAM 시험 (35장)
 *   I2C           BMP280(0x76)이 대답하는가 (15장)
 *   GPIO          지그의 선으로 이어 둔 PA0(출력) → PA1(입력) 되읽기
 *   SERIAL <값>   일련번호를 Flash 섹터 4에 기록 (프로비저닝)
 *   SERIAL?       기록된 일련번호 읽기
 * 시험이 끝나면 이 펌웨어를 지우고 제품 펌웨어를 올린다(또는 같은 펌웨어의 공장 모드로 남겨 둔다).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "stm32f411_regs.h"
#include "uart2.h"
#include "i2c.h"
#include "march.h"
#include "flash_if.h"
#include "crc32.h"

#define UID_ADDR      0x1FFF7A10u     /* 96비트 고유 번호 (RM0383 "Device electronic signature" 장) */
#define FLASHSIZE_REG 0x1FFF7A22u     /* Flash 크기 KB (같은 장) */
#define FACTORY_ADDR  0x08010000u     /* 섹터 4: 공장 데이터 (32장 배치에서 비어 있던 곳) */
#define FACTORY_MAGIC 0x31544346u     /* 'F' 'C' 'T' '1' */

typedef struct {
    uint32_t magic;
    char     serial[24];
    uint32_t crc;
} factory_rec_t;

static uint32_t ram_area[1024];
static uint32_t rd(void *c, size_t i) { return ((volatile uint32_t *)c)[i]; }
static void wr(void *c, size_t i, uint32_t v) { ((volatile uint32_t *)c)[i] = v; }

static void cmd_id(void)
{
    const uint32_t *uid = (const uint32_t *)UID_ADDR;
    printf("{\"uid\":\"%08lX%08lX%08lX\",\"flash_kb\":%u,\"fw\":\"factory-1.0\"}\n",
           (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0],
           (unsigned)*(const uint16_t *)FLASHSIZE_REG);
}

static void cmd_ram(void)
{
    mem_if_t m = { rd, wr, ram_area };
    long bad = march_c_minus(&m, sizeof ram_area / sizeof ram_area[0]);
    printf("{\"ram\":\"%s\"}\n", bad < 0 ? "ok" : "fail");
}

static void cmd_i2c(void)
{
    printf("{\"bmp280\":\"%s\"}\n", i2c_probe(0x76) == I2C_OK ? "ok" : "missing");
}

static void cmd_gpio(void)
{
    /* PA0 출력, PA1 입력(풀다운). 지그에서 두 핀이 선으로 이어져 있어야 한다 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << 0) & ~(3u << 2)) | (1u << 0);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << 2)) | (2u << 2);
    bool ok = true;
    for (int level = 1; level >= 0; level--) {
        GPIOA->BSRR = level ? (1u << 0) : (1u << 16);
        for (volatile int i = 0; i < 1000; i++) {
        }
        if (((GPIOA->IDR >> 1) & 1u) != (uint32_t)level) ok = false;
    }
    printf("{\"gpio\":\"%s\"}\n", ok ? "ok" : "fail");
}

static void cmd_serial_write(const char *s)
{
    factory_rec_t r;
    memset(&r, 0, sizeof r);
    r.magic = FACTORY_MAGIC;
    strncpy(r.serial, s, sizeof r.serial - 1);
    r.crc = crc32_update(0, &r, offsetof(factory_rec_t, crc));
    bool ok = flash_erase(FACTORY_ADDR) == 0 && flash_write(FACTORY_ADDR, &r, sizeof r) == 0;
    printf("{\"serial\":\"%s\"}\n", ok ? "written" : "error");
}

static void cmd_serial_read(void)
{
    const factory_rec_t *r = (const factory_rec_t *)FACTORY_ADDR;
    bool ok = r->magic == FACTORY_MAGIC && crc32_update(0, r, offsetof(factory_rec_t, crc)) == r->crc;
    printf("{\"serial\":\"%s\"}\n", ok ? r->serial : "");
}

int main(void)
{
    uart2_init(115200, 16000000u);
    setvbuf(stdout, NULL, _IONBF, 0);
    i2c_init(100000);
    printf("{\"ready\":true}\n");

    char line[48];
    size_t n = 0;
    for (;;) {
        int c = uart2_getc();
        if (c < 0) continue;
        if (c == '\r') continue;
        if (c != '\n') {
            if (n < sizeof line - 1) line[n++] = (char)c;   /* 너무 긴 줄은 잘라낸다 (14장) */
            continue;
        }
        line[n] = '\0';
        n = 0;
        if (strcmp(line, "ID") == 0) cmd_id();
        else if (strcmp(line, "RAM") == 0) cmd_ram();
        else if (strcmp(line, "I2C") == 0) cmd_i2c();
        else if (strcmp(line, "GPIO") == 0) cmd_gpio();
        else if (strncmp(line, "SERIAL ", 7) == 0) cmd_serial_write(line + 7);
        else if (strcmp(line, "SERIAL?") == 0) cmd_serial_read();
        else printf("{\"error\":\"unknown command\"}\n");
    }
}
