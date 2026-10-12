/*
 * 29장 실습: Zephyr로 같은 일을 다시 하기 (NUCLEO-F411RE)
 *
 *   blink 스레드   LD2를 500 ms마다 토글             (devicetree 별칭 led0)
 *   sensor 스레드  1초마다 BMP280을 읽어 메시지 큐로   (센서 드라이버 API)
 *   main 스레드    큐에서 꺼내 출력                     (k_msgq)
 *   B1 버튼        입력 콜백에서 측정을 멈추고 다시 켬   (gpio-keys + 입력 서브시스템)
 *
 * 핀 번호나 I2C 주소는 코드에 없다. 모두 devicetree(보드 파일 + boards/nucleo_f411re.overlay)에서 온다.
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>

/* devicetree에서 장치 가져오기: 빌드할 때 확정되며, 없는 노드면 빌드 오류 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *const bmp = DEVICE_DT_GET(DT_NODELABEL(bmp280));

struct sample {
	uint32_t   seq;
	uint32_t   uptime_ms;
	struct sensor_value temp;     /* val1 = 정수부, val2 = 백만분의 1 */
	struct sensor_value press;    /* kPa */
};

K_MSGQ_DEFINE(sample_q, sizeof(struct sample), 8, 4);

static atomic_t paused;

/* ---- B1: gpio-keys 드라이버가 디바운싱한 뒤 입력 이벤트로 알려 준다 ---- */
static void button_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);
	if (evt->code == INPUT_KEY_0 && evt->value == 1) {    /* 눌림 */
		bool now = !atomic_get(&paused);
		atomic_set(&paused, now);
		printk("sampling %s\n", now ? "paused" : "resumed");
	}
}
INPUT_CALLBACK_DEFINE(NULL, button_cb, NULL);

/* ---- 스레드 ---- */
static void blink_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	if (!gpio_is_ready_dt(&led)) {
		return;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	for (;;) {
		gpio_pin_toggle_dt(&led);
		k_msleep(500);
	}
}

static void sensor_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	if (!device_is_ready(bmp)) {
		printk("BMP280 not ready (wiring? address?)\n");
		return;
	}
	uint32_t seq = 0;
	int64_t next = k_uptime_get();
	for (;;) {
		next += 1000;
		k_sleep(K_TIMEOUT_ABS_MS(next));            /* 절대 시각까지: vTaskDelayUntil과 같은 역할 */
		if (atomic_get(&paused)) {
			continue;
		}
		struct sample s = { .seq = seq++, .uptime_ms = k_uptime_get_32() };
		if (sensor_sample_fetch(bmp) < 0) {
			printk("fetch failed\n");
			continue;
		}
		sensor_channel_get(bmp, SENSOR_CHAN_AMBIENT_TEMP, &s.temp);
		sensor_channel_get(bmp, SENSOR_CHAN_PRESS, &s.press);
		if (k_msgq_put(&sample_q, &s, K_NO_WAIT) != 0) {
			printk("queue full, sample %u dropped\n", s.seq);
		}
	}
}

/* 스택 크기는 바이트 단위, 우선순위는 숫자가 작을수록 높다 (FreeRTOS와 반대) */
K_THREAD_DEFINE(blink_tid,  512,  blink_thread,  NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	struct sample s;
	printk("ch29 Zephyr on %s: B1 pauses sampling\n", CONFIG_BOARD);
	for (;;) {
		k_msgq_get(&sample_q, &s, K_FOREVER);    /* 값이 올 때까지 기다림 */
		/* 온도: val1 + val2/1e6 °C, 기압: kPa → hPa로 보여 준다 */
		int32_t t_centi = s.temp.val1 * 100 + s.temp.val2 / 10000;
		int32_t p_deci_hpa = s.press.val1 * 100 + s.press.val2 / 10000;   /* kPa×100 = hPa×10 */
		printk("#%u %u ms  T = %s%d.%02d C  P = %d.%d hPa\n", s.seq, s.uptime_ms,
		       t_centi < 0 ? "-" : "", abs(t_centi) / 100, abs(t_centi) % 100,
		       p_deci_hpa / 10, p_deci_hpa % 10);
	}
	return 0;
}
