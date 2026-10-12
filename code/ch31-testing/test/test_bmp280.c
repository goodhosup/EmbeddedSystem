/*
 * 31장: 15장 BMP280 드라이버를 I2C 목으로 시험한다
 *   드라이버가 어떤 레지스터를 어떤 순서로 읽고 쓰는지, 오류를 어떻게 전하는지 확인한다.
 */
#include "unity.h"
#include "bmp280.h"
#include "i2c.h"
#include "mock_i2c.h"

static bmp280_t dev;

/* 데이터시트의 예시 보정값: dig_T1 = 27504, dig_T2 = 26435, dig_T3 = -1000 (리틀 엔디언) */
static const uint8_t calib[6] = { 0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC };

static void test_init_reads_id_and_calibration_then_configures(void)
{
    const uint8_t id = 0x58;
    mock_i2c_init();
    i2c_write_read_ExpectAndReturn(0x76, 0xD0, &id, 1, I2C_OK);
    i2c_write_read_ExpectAndReturn(0x76, 0x88, calib, 6, I2C_OK);
    i2c_write_ExpectAndReturn(0x76, 0xF5, 0x00, I2C_OK);
    i2c_write_ExpectAndReturn(0x76, 0xF4, 0x27, I2C_OK);    /* osrs_t ×1, osrs_p ×1, 노멀 */

    TEST_ASSERT_EQUAL_INT(0, bmp280_init(&dev, 0x76));
    TEST_ASSERT_EQUAL_UINT16(27504, dev.dig_t1);
    TEST_ASSERT_EQUAL_INT16(26435, dev.dig_t2);
    TEST_ASSERT_EQUAL_INT16(-1000, dev.dig_t3);
    mock_i2c_verify();
}

static void test_init_rejects_wrong_chip(void)
{
    const uint8_t id = 0x55;
    mock_i2c_init();
    i2c_write_read_ExpectAndReturn(0x76, 0xD0, &id, 1, I2C_OK);   /* 그 뒤로는 아무것도 하지 않아야 한다 */
    TEST_ASSERT_EQUAL_INT(-10, bmp280_init(&dev, 0x76));
    mock_i2c_verify();
}

static void test_init_passes_nack_through(void)
{
    mock_i2c_init();
    i2c_write_read_ExpectAndReturn(0x76, 0xD0, NULL, 1, I2C_ERR_NACK);
    TEST_ASSERT_EQUAL_INT(I2C_ERR_NACK, bmp280_init(&dev, 0x76));
    mock_i2c_verify();
}

static void test_temperature_matches_datasheet_example(void)
{
    /* 데이터시트 예: adc_T = 519888 → 25.08 °C */
    dev.addr = 0x76; dev.dig_t1 = 27504; dev.dig_t2 = 26435; dev.dig_t3 = -1000;
    const uint8_t raw[3] = { 0x7E, 0xED, 0x00 };             /* 519888 = 0x7EED0 */
    int32_t t;
    mock_i2c_init();
    i2c_write_read_ExpectAndReturn(0x76, 0xFA, raw, 3, I2C_OK);
    TEST_ASSERT_EQUAL_INT(0, bmp280_read_temp_c100(&dev, &t));
    TEST_ASSERT_EQUAL_INT32(2508, t);
    mock_i2c_verify();
}

void run_bmp280_tests(void)
{
    UnitySetTestFile(__FILE__);                    /* 결과에 이 파일 이름이 나오게 */
    RUN_TEST(test_init_reads_id_and_calibration_then_configures);
    RUN_TEST(test_init_rejects_wrong_chip);
    RUN_TEST(test_init_passes_nack_through);
    RUN_TEST(test_temperature_matches_datasheet_example);
}
