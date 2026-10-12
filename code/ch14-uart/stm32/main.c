/*
 * 14장 실습: UART 명령어 셸 (NUCLEO-F411RE)
 *
 * USART2: PA2 = TX, PA3 = RX (AF7). ST-LINK의 가상 COM 포트로 PC와 연결된다.
 * 115200 bps 8N1. 수신은 RXNE 인터럽트 + 링 버퍼, 송신은 폴링.
 * printf는 newlib-nano를 쓰고(USE_NEWLIB=1), __io_putchar()를 UART로 보낸다.
 *
 * 명령: help, led on|off, adc, uptime, echo <text>
 *
 * 레지스터: RM0383 19.6절 USART registers
 * 보레이트: OVER8 = 0이면 BRR = fCK / baud (반올림). 16 MHz / 115200 = 138.9 → 139
 */
#include <stdio.h>
#include <string.h>

#include "stm32f411_regs.h"

#define PCLK1_HZ  16000000u
#define RX_SIZE   64u
#define LINE_MAX  32

static volatile uint8_t  rx_buf[RX_SIZE];
static volatile uint32_t rx_head, rx_tail;
static volatile uint32_t rx_overflow;
static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t c = (uint8_t)USART2->DR;        /* SR을 읽은 뒤 DR을 읽으면 RXNE, ORE가 지워진다 */
        if (sr & (USART_SR_FE | USART_SR_PE)) {
            return;
        }
        uint32_t next = (rx_head + 1u) & (RX_SIZE - 1u);
        if (next == rx_tail) {
            rx_overflow = 1;
            return;
        }
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}

static void uart_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    gpio_set_af(GPIOA, 2, 7);                    /* PA2 = USART2_TX */
    gpio_set_af(GPIOA, 3, 7);                    /* PA3 = USART2_RX */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << (3 * 2))) | (1u << (3 * 2));  /* RX 풀업 */

    USART2->BRR = (PCLK1_HZ + baud / 2u) / baud;  /* 반올림 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;   /* 8N1, OVER8 = 0 */
    USART2->CR1 |= USART_CR1_UE;

    nvic_set_priority(USART2_IRQn, 5);
    nvic_enable_irq(USART2_IRQn);
}

static void uart_putc(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {    /* 송신 데이터 레지스터가 빌 때까지 */
    }
    USART2->DR = (uint8_t)c;
}

static int uart_getc(void)
{
    if (rx_head == rx_tail) {
        return -1;
    }
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_SIZE - 1u);
    return c;
}

/* newlib의 printf → _write() → __io_putchar() (code/common/stm32f411/syscalls.c) */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart_putc('\r');
    }
    uart_putc((char)ch);
    return ch;
}

static uint16_t adc_read0(void)
{
    ADC1->SQR3 = 0;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {
    }
    return (uint16_t)ADC1->DR;
}

static void run_command(char *line)
{
    char *cmd = strtok(line, " ");
    char *arg = strtok(NULL, "");

    if (cmd == NULL) {
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        printf("help, led on|off, adc, uptime, echo <text>\n");
    } else if (strcmp(cmd, "led") == 0 && arg && strcmp(arg, "on") == 0) {
        GPIOA->BSRR = 1u << 5;
        printf("LED on\n");
    } else if (strcmp(cmd, "led") == 0 && arg && strcmp(arg, "off") == 0) {
        GPIOA->BSRR = 1u << (5 + 16);
        printf("LED off\n");
    } else if (strcmp(cmd, "adc") == 0) {
        uint16_t v = adc_read0();
        printf("PA0 = %u (%lu mV)\n", v, (unsigned long)v * 3300u / 4096u);
    } else if (strcmp(cmd, "uptime") == 0) {
        printf("%lu ms\n", (unsigned long)ms_ticks);
    } else if (strcmp(cmd, "echo") == 0) {
        printf("%s\n", arg ? arg : "");
    } else {
        printf("unknown command: %s (type help)\n", cmd);
    }
}

int main(void)
{
    char line[LINE_MAX];
    uint32_t len = 0;

    SYSTICK->LOAD = 16000u - 1u;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;

    uart_init(115200);

    GPIOA->MODER = (GPIOA->MODER & ~(3u << (5 * 2))) | (1u << (5 * 2));   /* LD2 */
    GPIOA->MODER |= (3u << (0 * 2));                                      /* PA0 아날로그 */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;
    ADC1->SMPR2 = 7u;                                                     /* 채널 0: 480 클럭 */
    ADC1->CR2 = ADC_CR2_ADON;

    setvbuf(stdout, NULL, _IONBF, 0);            /* 버퍼 없이 바로 출력 (줄바꿈을 기다리지 않음) */
    printf("\nch14 UART shell. type help\n> ");

    for (;;) {
        int c = uart_getc();
        if (c < 0) {
            continue;
        }
        if (c == '\r' || c == '\n') {
            if (len > 0) {
                line[len] = '\0';
                printf("\n");
                run_command(line);
                len = 0;
            }
            printf("> ");
        } else if ((c == '\b' || c == 0x7F) && len > 0) {
            len--;
            printf("\b \b");
        } else if (c >= ' ' && len < LINE_MAX - 1) {
            line[len++] = (char)c;
            uart_putc((char)c);
        }
        if (rx_overflow) {
            rx_overflow = 0;
            printf("\n[rx overflow]\n");
        }
    }
}
