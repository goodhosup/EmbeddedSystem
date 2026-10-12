/*
 * SPI 마스터: STM32F411 SPI1 (16장)
 *   SCK = PA5 (D13), MISO = PA6 (D12), MOSI = PA7 (D11): AF5,  CS = PB6 (D10): GPIO 출력
 *   PCLK2 = 16 MHz (리셋 기본 클럭). SCK = PCLK2 / 2^(BR+1)
 *
 * 레지스터: RM0383 20장 SPI (CR1, SR, DR)
 * 참고: PA5는 보드 LED(LD2)와 같은 핀이라, 통신하는 동안 LED가 희미하게 깜빡인다.
 */
#include "spi.h"
#include "stm32f411_regs.h"

#define PCLK2_HZ 16000000u

void spi_init(uint32_t sclk_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    gpio_set_af(GPIOA, 5, 5);
    gpio_set_af(GPIOA, 6, 5);
    gpio_set_af(GPIOA, 7, 5);
    GPIOA->OSPEEDR |= (2u << (5 * 2)) | (2u << (7 * 2));      /* SCK, MOSI: 빠른 신호 (6장) */

    GPIOB->BSRR = 1u << 6;                                     /* CS High 먼저 */
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (6 * 2))) | (1u << (6 * 2));

    uint32_t br = 0;                                           /* PCLK2/2부터 */
    while (br < 7 && (PCLK2_HZ >> (br + 1)) > sclk_hz) {
        br++;
    }
    /* 모드 0, MSB 먼저, 8비트, 마스터, 소프트웨어 NSS(SSM = 1, SSI = 1: MODF 방지) */
    SPI1->CR1 = SPI_CR1_MSTR | (br << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi_xfer(uint8_t out)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0) {
    }
    *(volatile uint8_t *)&SPI1->DR = out;                      /* 8비트로 쓴다 */
    while ((SPI1->SR & SPI_SR_RXNE) == 0) {                    /* 받은 바이트 = 전송 끝 */
    }
    return *(volatile uint8_t *)&SPI1->DR;
}

void spi_cs_low(void)
{
    GPIOB->BSRR = 1u << (6 + 16);
}

void spi_cs_high(void)
{
    while (SPI1->SR & SPI_SR_BSY) {                            /* 마지막 비트까지 나간 뒤 */
    }
    GPIOB->BSRR = 1u << 6;
}
