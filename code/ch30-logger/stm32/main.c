/*
 * 30장 프로젝트: 환경 모니터링 데이터 로거 (NUCLEO-F411RE, FreeRTOS)
 *
 * 하드웨어 (30.2절)
 *   BMP280   I2C1: SCL PB8 (D15), SDA PB9 (D14), 주소 0x76      (15장)
 *   SD 카드  SPI1: SCK PA5 (D13), MISO PA6 (D12), MOSI PA7 (D11), CS PB6 (D10)  (16장)
 *   USART2   ST-LINK 가상 COM 포트, 송신은 DMA                   (14장, 10장)
 *   ※ PA5는 LD2와 같은 핀이라 이 프로젝트에서는 LED를 쓰지 않는다.
 *
 * 태스크 (30.3절)
 *   sensor   우선순위 3  주기마다 BMP280을 읽어 rec_q로     (I2C와 BMP280의 유일한 소유자)
 *   logger   우선순위 2  rec_q에서 꺼내 SD 카드에 CSV로      (SPI, SD, FatFs의 유일한 소유자)
 *   console  우선순위 1  UART 명령
 *
 * 명령 (115200 bps)
 *   s  상태     f  지금 바로 f_sync     x  파일 닫기 (카드를 안전하게 뺄 때)
 *   r  새 파일로 다시 시작             p  측정 주기 1 s ↔ 10 s
 */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stm32f411_regs.h"
#include "uart_dma.h"
#include "i2c.h"
#include "bmp280.h"
#include "sd_spi.h"
#include "logger.h"

#define PCLK1_HZ 16000000u

/* logger 태스크가 받는 메시지: 기록 또는 명령 */
typedef enum { MSG_REC, MSG_FLUSH, MSG_CLOSE, MSG_RESTART } msg_type_t;
typedef struct {
    msg_type_t type;
    logrec_t   rec;
} msg_t;

static QueueHandle_t msg_q;
static volatile uint32_t period_ms = 1000;

/* 상태: 각 값은 한 태스크만 쓰고 console은 읽기만 한다 (32비트 읽기는 원자적) */
static volatile uint32_t st_samples, st_sensor_err, st_dropped, st_card_err;
static volatile bool     st_logging;
static logger_t lg;                       /* logger 태스크만 건드린다 */

uint32_t sd_millis(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* ---- sensor ---- */
static void sensor_task(void *arg)
{
    (void)arg;
    bmp280_t bmp;
    i2c_init(100000);
    bool sensor_ok = (bmp280_init(&bmp, BMP280_ADDR_LOW) == 0);
    uint32_t seq = 0;
    bool dropped_before = false;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(period_ms));
        msg_t m = { .type = MSG_REC };
        m.rec.seq = seq++;
        m.rec.uptime_s = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
        if (!sensor_ok) {
            sensor_ok = (bmp280_init(&bmp, BMP280_ADDR_LOW) == 0);   /* 다시 시도: 커넥터가 헐거웠을 수도 */
        }
        if (!sensor_ok || bmp280_read_temp_c100(&bmp, &m.rec.temp_c100) != 0) {
            m.rec.flags |= LOGREC_F_SENSOR_ERR;
            st_sensor_err++;
            sensor_ok = false;
            i2c_bus_recover();                                        /* 15장의 버스 복구 */
        }
        if (dropped_before) {
            m.rec.flags |= LOGREC_F_DROPPED;
        }
        /* 기다리지 않는다: SD 카드가 느려도 측정 주기는 지킨다. 대신 버린 사실을 다음 기록에 남긴다 */
        dropped_before = (xQueueSend(msg_q, &m, 0) != pdPASS);
        if (dropped_before) {
            st_dropped++;
        }
        st_samples++;
    }
}

/* ---- logger ---- */
static bool try_start(void)
{
    FRESULT fr = logger_start(&lg, 10);      /* 10줄마다 f_sync */
    st_logging = (fr == FR_OK);
    if (st_logging) {
        log_printf("logging to %s\n", lg.name);
    } else {
        st_card_err++;
        log_printf("SD card not ready (FatFs error %d)\n", (int)fr);
    }
    return st_logging;
}

static void logger_task(void *arg)
{
    (void)arg;
    msg_t m;
    TickType_t last_try = 0;
    try_start();
    for (;;) {
        /* 기록하지 못하는 동안에는 5초마다 다시 마운트를 시도한다 */
        TickType_t wait = st_logging ? portMAX_DELAY : pdMS_TO_TICKS(5000);
        if (xQueueReceive(msg_q, &m, wait) != pdPASS) {
            if (xTaskGetTickCount() - last_try >= pdMS_TO_TICKS(5000)) {
                last_try = xTaskGetTickCount();
                try_start();
            }
            continue;
        }
        switch (m.type) {
        case MSG_REC:
            if (st_logging && logger_write(&lg, &m.rec) != FR_OK) {
                st_card_err++;
                log_printf("write failed (FatFs %d): card removed or full?\n", (int)lg.last_err);
                logger_stop(&lg);
                st_logging = false;
            }
            break;
        case MSG_FLUSH:
            if (st_logging) {
                log_printf("flush: %s\n", logger_flush(&lg) == FR_OK ? "ok" : "failed");
            }
            break;
        case MSG_CLOSE:
            logger_stop(&lg);
            st_logging = false;
            last_try = xTaskGetTickCount() + pdMS_TO_TICKS(60000);   /* 1분 동안 다시 열지 않음: 카드 교체 시간 */
            log_printf("file closed, you can remove the card (r to restart)\n");
            break;
        case MSG_RESTART:
            logger_stop(&lg);
            try_start();
            break;
        }
    }
}

/* ---- console ---- */
static void send_cmd(msg_type_t t)
{
    msg_t m = { .type = t };
    if (xQueueSend(msg_q, &m, pdMS_TO_TICKS(100)) != pdPASS) {
        log_printf("busy, try again\n");
    }
}

static void console_task(void *arg)
{
    (void)arg;
    char c;
    log_printf("\nch30 data logger: s=status f=flush x=close r=restart p=period\n");
    for (;;) {
        xQueueReceive(uart_rx_queue(), &c, portMAX_DELAY);
        switch (c) {
        case 's':
            log_printf("file %s %s, lines %lu, period %lu ms\n", st_logging ? lg.name : "-",
                       st_logging ? "open" : "closed", (unsigned long)lg.lines, (unsigned long)period_ms);
            log_printf("samples %lu, sensor errors %lu, dropped %lu, card errors %lu, queue %lu\n",
                       (unsigned long)st_samples, (unsigned long)st_sensor_err, (unsigned long)st_dropped,
                       (unsigned long)st_card_err, (unsigned long)uxQueueMessagesWaiting(msg_q));
            log_printf("heap free %u (min %u) B\n", (unsigned)xPortGetFreeHeapSize(),
                       (unsigned)xPortGetMinimumEverFreeHeapSize());
            break;
        case 'f': send_cmd(MSG_FLUSH); break;
        case 'x': send_cmd(MSG_CLOSE); break;
        case 'r': send_cmd(MSG_RESTART); break;
        case 'p':
            period_ms = (period_ms == 1000u) ? 10000u : 1000u;
            log_printf("period %lu ms\n", (unsigned long)period_ms);
            break;
        default:
            break;
        }
    }
}

/* ---- 훅 ---- */
void vApplicationIdleHook(void)
{
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    taskDISABLE_INTERRUPTS();
    uart_panic_puts("\r\n*** stack overflow: ");
    uart_panic_puts(name);
    uart_panic_puts(" ***\r\n");
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    uart_panic_puts("\r\n*** heap exhausted ***\r\n");
    for (;;) {
    }
}

void freertos_assert_failed(const char *file, int line)
{
    (void)line;
    taskDISABLE_INTERRUPTS();
    uart_panic_puts("\r\n*** configASSERT: ");
    uart_panic_puts(file);
    uart_panic_puts(" ***\r\n");
    __asm volatile ("bkpt #0");
    for (;;) {
    }
}

int __io_putchar(int ch)                 /* printf를 실수로 부르면 폴링으로라도 나가게 (log_printf를 쓸 것) */
{
    char str[2] = { (char)ch, '\0' };
    uart_panic_puts(str);
    return ch;
}

int main(void)
{
    msg_q = xQueueCreate(16, sizeof(msg_t));     /* 1 s 주기면 SD가 16초 멈춰도 잃지 않는다 */
    uart_dma_init(115200, PCLK1_HZ);

    /* 스택: 워드 단위. FatFs와 vsnprintf를 쓰는 태스크는 넉넉히 (s 명령과 25장의 방법으로 확인) */
    xTaskCreate(sensor_task,  "sensor",  384, NULL, 3, NULL);
    xTaskCreate(logger_task,  "logger",  768, NULL, 2, NULL);
    xTaskCreate(console_task, "console", 512, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
