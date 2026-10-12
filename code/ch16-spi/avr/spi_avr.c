/*
 * SPI 마스터: ATmega328P (16장)
 *   SCK = D13 (PB5), MISO = D12 (PB4), MOSI = D11 (PB3), CS = D10 (PB2)
 *
 * 레지스터: ATmega328P 데이터시트 SPI 장 (SPCR, SPSR, SPDR)
 *   SCK = F_CPU / {4, 16, 64, 128} (SPR1:0), SPI2X = 1이면 2배
 *
 * 주의: 마스터 모드에서 SS 핀(PB2)이 입력이고 Low가 되면 칩이 슬레이브 모드로 바뀐다.
 *       그래서 PB2를 반드시 출력으로 둔다. 여기서는 PB2를 CS로 쓴다.
 * 주의: SPI 플래시는 3.3 V 칩이다. 5 V 우노와 연결하려면 레벨 변환이 필요하다(16.6절).
 */
#include "spi.h"

#include <avr/io.h>

void spi_init(uint32_t sclk_hz)
{
    DDRB |= (1 << DDB5) | (1 << DDB3) | (1 << DDB2);     /* SCK, MOSI, SS(CS): 출력 */
    DDRB &= ~(1 << DDB4);                                 /* MISO: 입력 */
    PORTB |= (1 << PORTB2);                               /* CS High (선택 안 함) */

    /* 모드 0, MSB 먼저, 마스터. 요청 속도 이하가 되는 가장 빠른 분주를 고른다 */
    uint8_t spcr = (1 << SPE) | (1 << MSTR);
    uint8_t spsr = 0;
    if (sclk_hz >= F_CPU / 2) {
        spsr = (1 << SPI2X);                              /* F_CPU / 2 */
    } else if (sclk_hz >= F_CPU / 4) {
        /* F_CPU / 4 */
    } else if (sclk_hz >= F_CPU / 16) {
        spcr |= (1 << SPR0);
    } else if (sclk_hz >= F_CPU / 64) {
        spcr |= (1 << SPR1);
    } else {
        spcr |= (1 << SPR1) | (1 << SPR0);                /* F_CPU / 128 */
    }
    SPCR = spcr;
    SPSR = spsr;
}

uint8_t spi_xfer(uint8_t out)
{
    SPDR = out;                                           /* 쓰는 순간 8클럭이 나간다 */
    while ((SPSR & (1 << SPIF)) == 0) {                   /* 다 끝나면 SPIF = 1 */
    }
    return SPDR;                                          /* 그사이 들어온 바이트 */
}

void spi_cs_low(void)
{
    PORTB &= ~(1 << PORTB2);
}

void spi_cs_high(void)
{
    PORTB |= (1 << PORTB2);
}
