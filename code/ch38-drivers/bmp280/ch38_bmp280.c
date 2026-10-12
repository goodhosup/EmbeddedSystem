// SPDX-License-Identifier: GPL-2.0
/*
 * 38장: BMP280 온도를 IIO 장치로 내보내는 I2C 드라이버 (교육용으로 줄인 것)
 *
 *   디바이스 트리 노드의 compatible = "book,ch38-bmp280"과 짝지어진다 (ch38-bmp280-overlay.dts).
 *   리눅스 본류에는 이미 완전한 bmp280 IIO 드라이버(bosch,bmp280)가 있다. 이름을 다르게 한 것은
 *   그 드라이버와 겹치지 않게 하려는 것이다.
 *
 *   cat /sys/bus/iio/devices/iio:deviceN/in_temp_input   → 밀리도 (24870 = 24.87 °C)
 *
 * 보정 계산은 15장 bmp280.c와 같은 Bosch 데이터시트의 정수 식이다.
 */
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>

#define REG_CALIB   0x88
#define REG_CHIP_ID 0xD0
#define REG_CTRL    0xF4
#define REG_CONFIG  0xF5
#define REG_TEMP    0xFA

struct ch38_bmp280 {
	struct i2c_client *client;
	u16 t1;
	s16 t2, t3;
};

static int read_temp_milli(struct ch38_bmp280 *d, int *milli)
{
	u8 b[3];
	s32 adc, var1, var2, dt, t_fine;
	int ret = i2c_smbus_read_i2c_block_data(d->client, REG_TEMP, 3, b);

	if (ret < 0)
		return ret;
	if (ret != 3)
		return -EIO;
	adc = ((s32)b[0] << 12) | ((s32)b[1] << 4) | (b[2] >> 4);
	var1 = (((adc >> 3) - ((s32)d->t1 << 1)) * (s32)d->t2) >> 11;
	dt = (adc >> 4) - (s32)d->t1;
	var2 = (((dt * dt) >> 12) * (s32)d->t3) >> 14;
	t_fine = var1 + var2;
	*milli = ((t_fine * 5 + 128) >> 8) * 10;   /* 0.01 °C → 밀리도 (IIO의 온도 단위) */
	return 0;
}

static int ch38_read_raw(struct iio_dev *indio, struct iio_chan_spec const *chan,
			 int *val, int *val2, long mask)
{
	struct ch38_bmp280 *d = iio_priv(indio);
	int ret;

	if (mask != IIO_CHAN_INFO_PROCESSED)
		return -EINVAL;
	ret = read_temp_milli(d, val);
	return ret ? ret : IIO_VAL_INT;
}

static const struct iio_chan_spec ch38_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),   /* in_temp_input 파일이 생긴다 */
	},
};

static const struct iio_info ch38_info = {
	.read_raw = ch38_read_raw,
};

static int ch38_probe(struct i2c_client *client)
{
	struct iio_dev *indio;
	struct ch38_bmp280 *d;
	u8 c[6];
	int id, ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
		return dev_err_probe(&client->dev, -EOPNOTSUPP, "adapter lacks SMBus block read\n");

	id = i2c_smbus_read_byte_data(client, REG_CHIP_ID);
	if (id < 0)
		return dev_err_probe(&client->dev, id, "no answer at 0x%02x\n", client->addr);
	if (id != 0x58 && id != 0x60)
		return dev_err_probe(&client->dev, -ENODEV, "unexpected chip id 0x%02x\n", id);

	indio = devm_iio_device_alloc(&client->dev, sizeof(*d));   /* devm_: 장치가 떨어지면 자동 해제 */
	if (!indio)
		return -ENOMEM;
	d = iio_priv(indio);
	d->client = client;

	ret = i2c_smbus_read_i2c_block_data(client, REG_CALIB, 6, c);
	if (ret != 6)
		return dev_err_probe(&client->dev, ret < 0 ? ret : -EIO, "calibration read failed\n");
	d->t1 = (u16)(c[0] | (c[1] << 8));
	d->t2 = (s16)(c[2] | (c[3] << 8));
	d->t3 = (s16)(c[4] | (c[5] << 8));

	/* config: 필터 끔, ctrl_meas: 온도 ×1, 기압 ×1, 노멀 모드 (15장과 같음) */
	ret = i2c_smbus_write_byte_data(client, REG_CONFIG, 0x00);
	if (!ret)
		ret = i2c_smbus_write_byte_data(client, REG_CTRL, 0x27);
	if (ret)
		return ret;

	indio->name = "ch38_bmp280";
	indio->info = &ch38_info;
	indio->channels = ch38_channels;
	indio->num_channels = ARRAY_SIZE(ch38_channels);
	indio->modes = INDIO_DIRECT_MODE;
	ret = devm_iio_device_register(&client->dev, indio);
	if (!ret)
		dev_info(&client->dev, "chip id 0x%02x, IIO device registered\n", id);
	return ret;
}

static const struct of_device_id ch38_of_match[] = {
	{ .compatible = "book,ch38-bmp280" },
	{ }
};
MODULE_DEVICE_TABLE(of, ch38_of_match);      /* 이 표로 커널이 노드와 드라이버를 짝짓고, 모듈을 자동으로 올린다 */

static struct i2c_driver ch38_driver = {
	.driver = {
		.name = "ch38_bmp280",
		.of_match_table = ch38_of_match,
	},
	.probe = ch38_probe,
};
module_i2c_driver(ch38_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Embedded systems book, chapter 38: minimal BMP280 temperature IIO driver");
