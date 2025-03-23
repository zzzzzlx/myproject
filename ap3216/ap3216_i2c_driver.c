#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include "ap3216creg.h"

enum inv_ap3216c_scan {
	AP3216C_ALS,
	AP3216C_PS,
	AP3216C_IR,
};

struct ap3216c_dev {
	struct i2c_client *client;
	struct regmap *regmap;
	struct mutex lock;
};

static const int als_scale_ap3216c[] = {315000, 78800, 19700, 4900};

static int ap3216c_write_als_scale(struct ap3216c_dev *dev, int chann2, int val)
{
	int ret = 0, i;	
	u8 d;

	switch (chann2) {
	case IIO_MOD_LIGHT_BOTH:	/* 设置ALS分辨率 */
		for (i = 0; i < ARRAY_SIZE(als_scale_ap3216c); ++i) {
			if (als_scale_ap3216c[i] == val) {
				d = (i << 4);
				ret = regmap_write(dev->regmap, AP3216C_ALSCONFIG, d);
			}
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
		
	return ret;
}

static int ap3216c_read_alsir_data(struct ap3216c_dev *dev, int reg,
				   int chann2, int *val)
{
	int ret = 0;
	unsigned char data[2];

	switch (chann2)
	{
	case IIO_MOD_LIGHT_BOTH:
		ret = regmap_bulk_read(dev->regmap, reg, data, 2);
		*val = ((int)data[1] << 8) | data[0];
		break;
	case IIO_MOD_LIGHT_IR:
		ret = regmap_bulk_read(dev->regmap, reg, data, 2);
		*val = ((int)data[1] << 2) | (data[0] & 0X03); 
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret) {
		return -EINVAL;
	}
	return IIO_VAL_INT;
}


static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
	u8 ret;
	unsigned int data;
	ret = regmap_read(dev->regmap, reg, &data);
	return (u8)data;
}

static int ap3216c_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	int ret;
	unsigned char data[2];
	unsigned char regdata = 0;
	struct ap3216c_dev *dev = iio_priv(indio_dev);

	switch(mask){
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&dev->lock);
		switch(chan->type){
		case IIO_INTENSITY:
			ret = ap3216c_read_alsir_data(dev, chan->address, chan->channel2, val);
			break;
		case IIO_PROXIMITY:
			ret = regmap_bulk_read(dev->regmap, chan->address, data, 2);
			*val = ((int)(data[1] & 0X3F) << 4) | (data[0] & 0X0F);  
			ret = IIO_VAL_INT; 	/* 值为val */
			break;
		default:
			ret = -EINVAL;
			break;	
		}
		mutex_unlock(&dev->lock);
		return ret;
	case IIO_CHAN_INFO_SCALE:
		switch(chan->type){
		case IIO_INTENSITY:			/* ALS量程 */
			mutex_lock(&dev->lock);
			regdata = (ap3216c_read_reg(dev, AP3216C_ALSCONFIG) & 0X30) >> 4;
			*val  = 0;
			*val2 = als_scale_ap3216c[regdata];
			mutex_unlock(&dev->lock);
			return IIO_VAL_INT_PLUS_MICRO;	/* 值为val+val2/1000000 */
		default:
			return -EINVAL;
		}
		return ret;

	default:
		return -EINVAL;	
	}
	return ret;
}

static int ap3216c_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	int ret = 0;
	struct ap3216c_dev *dev = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_SCALE:	/* 设置ALS量程 */
		switch (chan->type) {
		case IIO_INTENSITY:		/* 设置ALS量程 */
			mutex_lock(&dev->lock);
			ret = ap3216c_write_als_scale(dev, chan->channel2, val2);
			mutex_unlock(&dev->lock);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int ap3216c_write_raw_get_fmt(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan, long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_INTENSITY:		/* 用户空间写的陀螺仪分辨率数据要乘以1000000 */
			return IIO_VAL_INT_PLUS_MICRO;
		default:				
			return IIO_VAL_INT_PLUS_MICRO;
		}
	default:
		return IIO_VAL_INT_PLUS_MICRO;
	}

	return -EINVAL;
}

static const struct iio_info ap3216c_info = {
	.read_raw = &ap3216c_read_raw,
	.write_raw = &ap3216c_write_raw,
	.write_raw_get_fmt = &ap3216c_write_raw_get_fmt,
	.driver_module = THIS_MODULE,
};

static const struct iio_chan_spec ap3216c_channels[] = {
	/* ALS通道 */
	{
		.type = IIO_INTENSITY,
		.modified = 1,
		.channel2 = IIO_MOD_LIGHT_BOTH,
		.address = AP3216C_ALSDATALOW,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = AP3216C_ALS,
		.scan_type = {
			.sign = 'u',
			.realbits = 16,
			.storagebits = 16,
			.endianness = IIO_LE,
		},
	},

	/* PS通道 */
	{
		.type = IIO_PROXIMITY,
		.address = AP3216C_PSDATALOW,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = AP3216C_PS,
		.scan_type = {
			.sign = 'u',
			.realbits = 10,
			.storagebits = 16,
			.endianness = IIO_LE,
		},
	},

	/* IR通道 */
	{
		.type = IIO_INTENSITY,
		.modified = 1,
		.channel2 = IIO_MOD_LIGHT_IR,
		.address = AP3216C_IRDATALOW,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = AP3216C_IR,
		.scan_type = {
			.sign = 'u',
			.realbits = 10,
			.storagebits = 16,
			.endianness = IIO_LE,
		},
	},
};

static const struct regmap_config ap3216c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
	regmap_write(dev->regmap, reg, data);
}

static int ap3216c_reginit(struct ap3216c_dev *dev)
{
	/* 初始化AP3216C */
	ap3216c_write_reg(dev, AP3216C_SYSTEMCONG, 0x04);		/* 复位AP3216C 			*/
	mdelay(50);												/* AP3216C复位最少10ms 	*/
	ap3216c_write_reg(dev, AP3216C_SYSTEMCONG, 0X03);		/* 开启ALS、PS+IR 		*/
	ap3216c_write_reg(dev, AP3216C_ALSCONFIG, 0X00);		/* ALS单次转换触发，量程为0～20661 lux */
	ap3216c_write_reg(dev, AP3216C_PSLEDCONFIG, 0X13);		/* IR LED 1脉冲，驱动电流100%*/

	return 0;
}

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ap3216c_dev *data;
	struct iio_dev *indio_dev;
	int ret = 0;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;
	
	data = iio_priv(indio_dev);
	data->client = client;
	mutex_init(&data->lock);

	i2c_set_clientdata(client, indio_dev);
	data->regmap = devm_regmap_init_i2c(client, &ap3216c_regmap_config);
	if (IS_ERR(data->regmap)) {
		dev_err(&client->dev, "Failed to initialize regmap\n");
		return -EINVAL;
	}

	indio_dev->info = &ap3216c_info;
	indio_dev->name = id->name;
	indio_dev->dev.parent = &client->dev;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ap3216c_channels;
	indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "unable to register iio device\n");
	}


	/* init ap3216c */
	ap3216c_reginit(data); /* 初始化ap3216c */

	return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	iio_device_unregister(indio_dev);
	return 0;
}

static const struct of_device_id ap3216c_dt_ids[] = {
	{ .compatible = "zlx,ap3216c" },
	{ }
};

static const struct i2c_device_id ap3216c_id[] = {
	{"zlx,ap3216c"},
	{ }
};

static struct i2c_driver ap3216c_driver = {
	.driver = {
		.name	= "ap3216c",
		.of_match_table = of_match_ptr(ap3216c_dt_ids),
	},
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);
MODULE_LICENSE("GPL");