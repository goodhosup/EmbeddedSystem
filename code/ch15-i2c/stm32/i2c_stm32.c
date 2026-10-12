/*
 * I2C 마스터: STM32F411 I2C1 (15장)
 *   SCL = PB8 (아두이노 헤더 D15), SDA = PB9 (D14), AF4, 오픈 드레인
 *   PCLK1 = 16 MHz (리셋 기본 클럭)
 *
 * 레지스터와 순서: RM0383 18장 I2C
 *   - 마스터 송신/수신 순서도와 "Closing the communication" 절의 수신 바이트 수(1, 2, 3 이상)별 절차
 *   - 표준 모드 100 kHz: CCR = PCLK1 / (2 × 100 kHz) = 80, TRISE = PCLK1(MHz) + 1 = 17
 *
 * ST의 HAL 드라이버도 같은 절차를 따른다. 플래그를 기다리는 곳에는 모두 시간 제한을 둔다.
 */
#include "i2c.h"
#include "stm32f411_regs.h"

#define PCLK1_MHZ  16u
#define TIMEOUT    100000u

static int wait_sr1(uint32_t mask)
{
    for (uint32_t t = 0; t < TIMEOUT; t++) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & I2C_SR1_AF) {                 /* ACK 실패 */
            I2C1->SR1 = ~I2C_SR1_AF;            /* rc_w0: 0을 쓴 비트만 지워진다 */
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERR_NACK;
        }
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO)) {
            I2C1->SR1 = ~(I2C_SR1_BERR | I2C_SR1_ARLO);
            return I2C_ERR_BUS;
        }
        if (sr1 & mask) {
            return I2C_OK;
        }
    }
    return I2C_ERR_TIMEOUT;
}

static int wait_not_busy(void)
{
    for (uint32_t t = 0; t < TIMEOUT; t++) {
        if ((I2C1->SR2 & I2C_SR2_BUSY) == 0) {
            return I2C_OK;
        }
    }
    return I2C_ERR_TIMEOUT;
}

static void wait_stop_done(void)
{
    for (uint32_t t = 0; t < TIMEOUT && (I2C1->CR1 & I2C_CR1_STOP); t++) {
    }
}

static void clear_addr(void)
{
    (void)I2C1->SR1;                            /* SR1을 읽고 */
    (void)I2C1->SR2;                            /* SR2를 읽으면 ADDR이 지워진다 */
}

/* 시작 조건 + 주소(R/W) 보내기. ADDR이 설정된 상태로 돌아온다(아직 지우지 않음) */
static int start_and_address(uint8_t addr_rw)
{
    I2C1->CR1 |= I2C_CR1_START;
    int r = wait_sr1(I2C_SR1_SB);
    if (r != I2C_OK) {
        return r;
    }
    I2C1->DR = addr_rw;                         /* SB는 SR1 읽기 + DR 쓰기로 지워진다 */
    return wait_sr1(I2C_SR1_ADDR);
}

static void pins_af(void)
{
    gpio_set_af(GPIOB, 8, 4);
    gpio_set_af(GPIOB, 9, 4);
    GPIOB->OTYPER |= (1u << 8) | (1u << 9);     /* 오픈 드레인 (2장, 15.2절) */
    GPIOB->PUPDR = (GPIOB->PUPDR & ~((3u << 16) | (3u << 18))) | (1u << 16) | (1u << 18);   /* 보조 풀업 */
}

void i2c_init(uint32_t scl_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;
    pins_af();

    I2C1->CR1 = I2C_CR1_SWRST;                  /* 주변장치 리셋 */
    I2C1->CR1 = 0;
    I2C1->CR2 = PCLK1_MHZ;                      /* FREQ = APB1 클럭(MHz) */
    if (scl_hz > 100000u) {
        /* 고속 모드(DUTY = 0): Thigh = CCR × TPCLK, Tlow = 2 × CCR × TPCLK */
        I2C1->CCR   = I2C_CCR_FS | ((PCLK1_MHZ * 1000000u + 3u * scl_hz - 1u) / (3u * scl_hz));
        I2C1->TRISE = PCLK1_MHZ * 300u / 1000u + 1u;          /* 최대 상승 시간 300 ns */
    } else {
        I2C1->CCR   = PCLK1_MHZ * 1000000u / (2u * scl_hz);   /* 100 kHz → 80 */
        I2C1->TRISE = PCLK1_MHZ + 1u;                         /* 최대 상승 시간 1000 ns */
    }
    I2C1->CR1 = I2C_CR1_PE;
}

static int write_bytes(const uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int r = wait_sr1(I2C_SR1_TXE);
        if (r != I2C_OK) {
            return r;
        }
        I2C1->DR = data[i];
    }
    return wait_sr1(I2C_SR1_BTF);               /* 마지막 바이트까지 다 나갔다 */
}

static int read_bytes(uint8_t addr, uint8_t *buf, size_t n)
{
    int r;

    if (n == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if ((r = start_and_address((uint8_t)((addr << 1) | 1u))) != I2C_OK) {
            return r;
        }
        uint32_t s = irq_save();                /* ADDR 지우기와 STOP 설정 사이에 끼어들지 않게 */
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;
        irq_restore(s);
        if ((r = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
            return r;
        }
        buf[0] = (uint8_t)I2C1->DR;
    } else if (n == 2) {
        I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_POS;    /* POS: ACK 비트가 "다음" 바이트에 적용 */
        if ((r = start_and_address((uint8_t)((addr << 1) | 1u))) != I2C_OK) {
            I2C1->CR1 &= ~I2C_CR1_POS;
            return r;
        }
        uint32_t s = irq_save();
        clear_addr();
        I2C1->CR1 &= ~I2C_CR1_ACK;                  /* 두 번째 바이트에 NACK */
        irq_restore(s);
        if ((r = wait_sr1(I2C_SR1_BTF)) != I2C_OK) { /* 두 바이트가 DR과 시프트 레지스터에 */
            I2C1->CR1 &= ~I2C_CR1_POS;
            return r;
        }
        s = irq_save();
        I2C1->CR1 |= I2C_CR1_STOP;
        buf[0] = (uint8_t)I2C1->DR;
        irq_restore(s);
        buf[1] = (uint8_t)I2C1->DR;
        I2C1->CR1 &= ~I2C_CR1_POS;
    } else {
        I2C1->CR1 |= I2C_CR1_ACK;
        if ((r = start_and_address((uint8_t)((addr << 1) | 1u))) != I2C_OK) {
            return r;
        }
        clear_addr();
        size_t i = 0;
        while (n - i > 3) {
            if ((r = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
                return r;
            }
            buf[i++] = (uint8_t)I2C1->DR;
        }
        /* 3바이트 남음: N-2가 DR에, N-1이 시프트 레지스터에 찰 때까지(BTF) 기다린다 */
        if ((r = wait_sr1(I2C_SR1_BTF)) != I2C_OK) {
            return r;
        }
        I2C1->CR1 &= ~I2C_CR1_ACK;                  /* 마지막 바이트에 NACK */
        buf[i++] = (uint8_t)I2C1->DR;               /* N-2 */
        if ((r = wait_sr1(I2C_SR1_BTF)) != I2C_OK) {
            return r;
        }
        uint32_t s = irq_save();
        I2C1->CR1 |= I2C_CR1_STOP;
        buf[i++] = (uint8_t)I2C1->DR;               /* N-1 */
        irq_restore(s);
        if ((r = wait_sr1(I2C_SR1_RXNE)) != I2C_OK) {
            return r;
        }
        buf[i++] = (uint8_t)I2C1->DR;               /* N */
    }
    wait_stop_done();
    return I2C_OK;
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t n)
{
    int r = wait_not_busy();
    if (r == I2C_OK) {
        r = start_and_address((uint8_t)(addr << 1));
    }
    if (r == I2C_OK) {
        clear_addr();
        r = write_bytes(data, n);
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    wait_stop_done();
    return r;
}

int i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wn, uint8_t *rdata, size_t rn)
{
    int r = wait_not_busy();
    if (r == I2C_OK) {
        r = start_and_address((uint8_t)(addr << 1));
    }
    if (r == I2C_OK) {
        clear_addr();
        r = write_bytes(wdata, wn);
    }
    if (r == I2C_OK) {
        r = read_bytes(addr, rdata, rn);            /* START를 다시 걸면 반복 시작이 된다 */
    }
    if (r != I2C_OK) {
        I2C1->CR1 |= I2C_CR1_STOP;
        wait_stop_done();
    }
    return r;
}

int i2c_probe(uint8_t addr)
{
    int r = wait_not_busy();
    if (r == I2C_OK) {
        r = start_and_address((uint8_t)(addr << 1));
        if (r == I2C_OK) {
            clear_addr();
        }
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    wait_stop_done();
    return r;
}

static void delay_us_rough(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * 4u; i++) {
    }
}

void i2c_bus_recover(void)
{
    I2C1->CR1 = 0;                                          /* 주변장치 끄기 */
    /* PB8(SCL), PB9(SDA)를 GPIO 오픈 드레인 출력으로 */
    GPIOB->BSRR  = (1u << 8) | (1u << 9);
    GPIOB->MODER = (GPIOB->MODER & ~((3u << 16) | (3u << 18))) | (1u << 16) | (1u << 18);
    for (int i = 0; i < 9 && (GPIOB->IDR & (1u << 9)) == 0; i++) {   /* SDA가 Low인 동안 */
        GPIOB->BSRR = 1u << (8 + 16);                       /* SCL Low */
        delay_us_rough(5);
        GPIOB->BSRR = 1u << 8;                              /* SCL 놓기 */
        delay_us_rough(5);
    }
    /* 정지 조건 흉내: SCL High 동안 SDA Low → High */
    GPIOB->BSRR = 1u << (9 + 16);
    delay_us_rough(5);
    GPIOB->BSRR = 1u << 9;
    delay_us_rough(5);
    i2c_init(100000);
}
