/*
 * 32장: STM32F411 내장 Flash 지우기/쓰기 (flash_if.h 구현)
 *   RM0383 3.5절 "Erase and program operations", 3.8절 레지스터
 *   - CR은 LOCK 상태로 시작: KEYR에 두 키를 차례로 써서 푼다
 *   - 32비트 쓰기(PSIZE = 10)는 전원 2.7~3.6 V에서. NUCLEO는 3.3 V
 *   - 지우거나 쓰는 동안 같은 뱅크의 Flash를 읽으면 CPU가 멈춰 기다린다(F411은 뱅크가 하나)
 *   - 섹터 0~1(부트로더)은 이 함수로 지울 수 없게 막아 둔다
 */
#include "flash_if.h"
#include "layout.h"
#include "stm32f411_regs.h"

static int sector_of(uint32_t a)
{
    static const uint32_t start[8] = { 0x08000000u, 0x08004000u, 0x08008000u, 0x0800C000u,
                                       0x08010000u, 0x08020000u, 0x08040000u, 0x08060000u };
    for (int i = 2; i < 8; i++) {          /* 0, 1은 부트로더: 거부 */
        if (start[i] == a) {
            return i;
        }
    }
    return -1;
}

static int wait_done(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {
    }
    uint32_t sr = FLASH->SR;
    FLASH->SR = sr & (FLASH_SR_ERRORS | FLASH_SR_EOP);    /* 1을 써서 지운다 */
    return (sr & FLASH_SR_ERRORS) ? -1 : 0;
}

static void unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void lock(void)
{
    FLASH->CR = FLASH_CR_LOCK;
}

int flash_erase(uint32_t sector_addr)
{
    int snb = sector_of(sector_addr);
    if (snb < 0) {
        return -1;
    }
    unlock();
    (void)wait_done();                                      /* 남아 있던 오류 플래그 지우기 */
    FLASH->CR = FLASH_CR_PSIZE_32 | FLASH_CR_SER | ((uint32_t)snb << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;                             /* 128 KB 섹터는 1~2초 걸린다 */
    int r = wait_done();
    lock();
    return r;
}

int flash_write(uint32_t addr, const void *data, uint32_t len)
{
    if ((addr & 3u) || (len & 3u) || addr < META0_ADDR || addr + len > FLASH_BASE_ADDR + FLASH_TOTAL) {
        return -1;
    }
    const uint8_t *d = (const uint8_t *)data;
    int r = 0;
    unlock();
    (void)wait_done();
    FLASH->CR = FLASH_CR_PSIZE_32 | FLASH_CR_PG;
    for (uint32_t i = 0; i < len && r == 0; i += 4) {
        uint32_t w = (uint32_t)d[i] | ((uint32_t)d[i + 1] << 8) | ((uint32_t)d[i + 2] << 16) | ((uint32_t)d[i + 3] << 24);
        *(volatile uint32_t *)(addr + i) = w;               /* 정렬되지 않은 data도 받도록 바이트로 조립 */
        r = wait_done();
    }
    lock();
    return r;
}

const void *flash_map(uint32_t addr)
{
    return (const void *)addr;                              /* 내장 Flash는 주소 그대로 읽힌다 */
}
