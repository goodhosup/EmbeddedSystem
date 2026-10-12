/*
 * 37장: 리눅스 사용자 공간에서 GPIO 다루기 (GPIO 문자 장치, uAPI v2)
 *
 *   LED:    GPIO17 (헤더 11번 핀) → 330 Ω → LED → GND
 *   버튼:   GPIO27 (헤더 13번 핀) ↔ 버튼 ↔ GND   (내부 풀업 사용)
 *
 * 버튼을 누를 때마다(하강 에지) 커널이 이벤트를 큐에 넣고, 이 프로그램은 read()로 기다린다.
 * 7장의 EXTI 인터럽트를 커널이 대신 처리해 주는 셈이다. 10번 누르면 끝난다.
 * 라이브러리(libgpiod) 없이 커널 헤더 <linux/gpio.h>만 쓴다.
 *
 *   make && ./gpio_button_led            (사용자가 gpio 그룹에 있어야 한다)
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define CHIP     "/dev/gpiochip0"   /* Raspberry Pi 5: 헤더 핀은 RP1의 gpiochip0 [확인 필요: 다른 모델·커널] */
#define LED_LINE 17
#define BTN_LINE 27

static int request_line(int chip, unsigned offset, __u64 flags, unsigned debounce_us, const char *who)
{
    struct gpio_v2_line_request req;
    memset(&req, 0, sizeof req);
    req.offsets[0] = offset;
    req.num_lines = 1;
    req.config.flags = flags;
    if (debounce_us) {                                     /* 7장의 디바운싱을 커널에 맡긴다 */
        req.config.num_attrs = 1;
        req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
        req.config.attrs[0].attr.debounce_period_us = debounce_us;
        req.config.attrs[0].mask = 1;                      /* 요청한 줄 중 0번 줄에 적용 */
    }
    strncpy(req.consumer, who, sizeof req.consumer - 1);   /* gpioinfo에 이 이름으로 보인다 */
    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        perror("GPIO_V2_GET_LINE_IOCTL");
        return -1;
    }
    return req.fd;                                         /* 이 파일 기술자로 그 핀을 다룬다 */
}

static int set_value(int line_fd, int on)
{
    struct gpio_v2_line_values v = { .bits = on ? 1u : 0u, .mask = 1u };
    return ioctl(line_fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &v);
}

int main(void)
{
    int chip = open(CHIP, O_RDWR | O_CLOEXEC);
    if (chip < 0) {
        perror(CHIP);
        return 1;
    }
    int led = request_line(chip, LED_LINE, GPIO_V2_LINE_FLAG_OUTPUT, 0, "ch37-led");
    int btn = request_line(chip, BTN_LINE, GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_BIAS_PULL_UP |
                           GPIO_V2_LINE_FLAG_EDGE_FALLING, 10000, "ch37-button");   /* 10 ms */
    close(chip);                                           /* 줄을 받았으면 칩은 닫아도 된다 */
    if (led < 0 || btn < 0) {
        return 1;
    }

    int on = 0;
    printf("press the button (GPIO%d) 10 times...\n", BTN_LINE);
    for (int presses = 0; presses < 10; ) {
        struct gpio_v2_line_event ev;
        ssize_t n = read(btn, &ev, sizeof ev);             /* 에지가 올 때까지 잠든다 (CPU 0%) */
        if (n < 0 && errno == EINTR) continue;
        if (n != (ssize_t)sizeof ev) {
            perror("read");
            break;
        }
        on = !on;
        set_value(led, on);
        presses++;
        printf("press %d at %llu.%09llu s (kernel timestamp), LED %s\n", presses,
               (unsigned long long)(ev.timestamp_ns / 1000000000ull),
               (unsigned long long)(ev.timestamp_ns % 1000000000ull), on ? "on" : "off");
    }
    set_value(led, 0);
    close(led);                                            /* 닫으면 핀은 커널에 돌아간다 */
    close(btn);
    return 0;
}
