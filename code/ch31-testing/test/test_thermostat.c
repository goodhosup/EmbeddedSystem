/*
 * 31장: thermostat 모듈의 단위 테스트 (Unity)
 *
 * 테스트 더블
 *   fake_read   스텁: 미리 정한 값(또는 실패)을 차례로 돌려준다
 *   spy_heater  스파이: 불린 횟수와 마지막 값을 기록한다
 * 이 파일은 PC(test/main_host.c)와 보드(stm32/main.c)에서 똑같이 실행된다.
 */
#include "unity.h"
#include "thermostat.h"

/* ---- 테스트 더블 ---- */
typedef struct { int ret; int32_t t; } reading_t;

static reading_t script[16];
static int script_len, script_pos;
static int  heater_calls;
static bool heater_on;

static int fake_read(void *ctx, int32_t *t)
{
    (void)ctx;
    TEST_ASSERT_TRUE_MESSAGE(script_pos < script_len, "sensor read more times than scripted");
    reading_t r = script[script_pos++];
    *t = r.t;
    return r.ret;
}

static void spy_heater(void *ctx, bool on)
{
    (void)ctx;
    heater_calls++;
    heater_on = on;
}

static const thermo_io_t io = { fake_read, spy_heater, NULL };
static thermostat_t th;

static void give(int32_t t)        { script[script_len++] = (reading_t){ 0, t }; }
static void give_error(void)       { script[script_len++] = (reading_t){ -1, 0 }; }
static void steps(int n)           { for (int i = 0; i < n; i++) thermo_step(&th); }

/* Unity가 각 테스트 앞뒤에 부른다 */
void setUp(void)
{
    script_len = script_pos = 0;
    heater_calls = 0;
    heater_on = true;                              /* init이 끄는지 확인하려고 일부러 true */
    thermo_init(&th, &io, 2000, 50);               /* 20.00 °C ± 0.50 */
}

void tearDown(void)
{
}

/* ---- 테스트: TDD로 하나씩 추가한 순서 그대로 ---- */
static void test_init_turns_heater_off(void)
{
    TEST_ASSERT_FALSE(heater_on);
    TEST_ASSERT_EQUAL_INT(1, heater_calls);
    TEST_ASSERT_EQUAL(TH_IDLE, thermo_state(&th));
}

static void test_heater_turns_on_below_band(void)
{
    give(1949);                                    /* 19.49 < 19.50 */
    steps(1);
    TEST_ASSERT_TRUE(heater_on);
    TEST_ASSERT_EQUAL(TH_HEATING, thermo_state(&th));
}

static void test_heater_does_not_turn_on_at_band_edge(void)
{
    give(1950);                                    /* 경계: 아직 켜지 않는다 */
    steps(1);
    TEST_ASSERT_FALSE(heater_on);
}

static void test_heater_stays_on_inside_band(void)
{
    give(1900); give(2000); give(2050);            /* 켜진 뒤 띠 안에서는 그대로 (히스테리시스) */
    steps(3);
    TEST_ASSERT_TRUE(heater_on);
    TEST_ASSERT_EQUAL_INT(2, heater_calls);        /* init 1번 + 켜기 1번: 같은 명령을 반복하지 않음 */
}

static void test_heater_turns_off_above_band(void)
{
    give(1900); give(2051);
    steps(2);
    TEST_ASSERT_FALSE(heater_on);
    TEST_ASSERT_EQUAL(TH_IDLE, thermo_state(&th));
}

static void test_two_failures_keep_current_state(void)
{
    give(1900); give_error(); give_error();
    steps(3);
    TEST_ASSERT_TRUE(heater_on);                   /* 잠깐의 실패로 히터를 껐다 켰다 하지 않는다 */
    TEST_ASSERT_EQUAL(TH_HEATING, thermo_state(&th));
}

static void test_three_failures_enter_fault_and_turn_heater_off(void)
{
    give(1900); give_error(); give_error(); give_error();
    steps(4);
    TEST_ASSERT_EQUAL(TH_FAULT, thermo_state(&th));
    TEST_ASSERT_FALSE(heater_on);
}

static void test_success_resets_failure_count(void)
{
    give_error(); give_error(); give(2000); give_error(); give_error();
    steps(5);
    TEST_ASSERT_EQUAL(TH_IDLE, thermo_state(&th));
}

static void test_out_of_range_reading_counts_as_failure(void)
{
    give(9000); give(-5000); give(12000);          /* 90 °C, -50 °C, 120 °C: 실내 센서로는 불가능 */
    steps(3);
    TEST_ASSERT_EQUAL(TH_FAULT, thermo_state(&th));
}

static void test_recovers_after_three_good_readings(void)
{
    give_error(); give_error(); give_error();      /* FAULT */
    give(1900); give(1900);
    steps(5);
    TEST_ASSERT_EQUAL(TH_FAULT, thermo_state(&th));    /* 아직 두 번 */
    give(1900);
    steps(1);
    TEST_ASSERT_EQUAL(TH_IDLE, thermo_state(&th));     /* 세 번째에 회복 */
    TEST_ASSERT_FALSE(heater_on);                      /* 회복한 주기에는 아직 켜지 않음 */
    give(1900);
    steps(1);
    TEST_ASSERT_TRUE(heater_on);
}

static void test_setpoint_range_is_checked(void)
{
    TEST_ASSERT_TRUE(thermo_set_setpoint(&th, 2500));
    TEST_ASSERT_FALSE(thermo_set_setpoint(&th, 499));
    TEST_ASSERT_FALSE(thermo_set_setpoint(&th, 3501));
    TEST_ASSERT_EQUAL_INT32(2500, th.setpoint_c100);   /* 거부된 값은 반영되지 않는다 */
}

void run_thermostat_tests(void)
{
    UnitySetTestFile(__FILE__);                    /* 결과에 이 파일 이름이 나오게 */
    RUN_TEST(test_init_turns_heater_off);
    RUN_TEST(test_heater_turns_on_below_band);
    RUN_TEST(test_heater_does_not_turn_on_at_band_edge);
    RUN_TEST(test_heater_stays_on_inside_band);
    RUN_TEST(test_heater_turns_off_above_band);
    RUN_TEST(test_two_failures_keep_current_state);
    RUN_TEST(test_three_failures_enter_fault_and_turn_heater_off);
    RUN_TEST(test_success_resets_failure_count);
    RUN_TEST(test_out_of_range_reading_counts_as_failure);
    RUN_TEST(test_recovers_after_three_good_readings);
    RUN_TEST(test_setpoint_range_is_checked);
}
