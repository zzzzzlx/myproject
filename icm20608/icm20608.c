#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include "icm20608reg.h"

#define ICM20608_NAME "icm20608"
#define ICM20608_TEMP_OFFSET	     0
#define ICM20608_TEMP_SCALE		     326800000

#define ICM20608_CHAN(_type, _channel2, _index)                 \
{                                                               \
        .type = _type,                                          \
        .modified = 1,                                          \
		.channel2 = _channel2,                                  \
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),   \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)            \
				| BIT(IIO_CHAN_INFO_CALIBBIAS),                 \
        .scan_index = _index,                                   \
        .scan_type = {                                          \
            .sign = 's',                                        \
            .realbits = 16,                                     \
            .storagebits = 16,                                  \
            .shift = 0,                                         \
            .endianness = IIO_BE,                               \
        },                                                      \
}

struct icm20608_dev{
    struct spi_device *spi;
    struct mutex lock;
    struct regmap *regmap;
};

enum inv_icm20608_scan {
	INV_ICM20608_SCAN_ACCL_X,
	INV_ICM20608_SCAN_ACCL_Y,
	INV_ICM20608_SCAN_ACCL_Z,
	INV_ICM20608_SCAN_TEMP,
	INV_ICM20608_SCAN_GYRO_X,
	INV_ICM20608_SCAN_GYRO_Y,
	INV_ICM20608_SCAN_GYRO_Z,
};

static const int accel_scale_icm20608[] = {61035, 122070, 244140, 488281};

static const int gyro_scale_icm20608[] = {7629, 15258, 30517, 61035};

static const struct iio_chan_spec icm20608_channels[] = {
    /* 温度通道 */
    {
        .type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
				| BIT(IIO_CHAN_INFO_OFFSET)
				| BIT(IIO_CHAN_INFO_SCALE),
        .scan_index = INV_ICM20608_SCAN_TEMP,
        .scan_type = {
            .sign = 's',
            .realbits = 16,
            .storagebits = 16,
            .shift = 0,
            .endianness = IIO_BE,
        },
    },

    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_X, INV_ICM20608_SCAN_ACCL_X),
    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_ICM20608_SCAN_ACCL_Y),
    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_ICM20608_SCAN_ACCL_Z),

    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_ICM20608_SCAN_GYRO_X),
    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_ICM20608_SCAN_GYRO_Y),
    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_ICM20608_SCAN_GYRO_Z),

};

static int icm20608_sensor_show(struct icm20608_dev *dev, int reg,
				   int axis, int *val)
{
    int index;
    __be16 data;

    index = (axis - IIO_MOD_X) * 2;

    regmap_bulk_read(dev->regmap, reg + index, (u8*)&data, 2);
    *val = (short)be16_to_cpup(&data);
	return IIO_VAL_INT;
}

static int icm20608_read_onereg(struct icm20608_dev *dev, unsigned int reg)
{
    unsigned int data;
    regmap_read(dev->regmap, reg, &data);
    return (u8)data;
}


static int icm20608_read_channel_data(struct iio_dev *indio_dev,
					 struct iio_chan_spec const *chan,
					 int *val)
{
    struct icm20608_dev *dev = iio_priv(indio_dev);
    int ret = 0;

    switch (chan->type){
    case IIO_ACCEL:
        ret = icm20608_sensor_show(dev, ICM20_ACCEL_XOUT_H, chan->channel2, val);
        break;
    case IIO_ANGL_VEL:
        ret = icm20608_sensor_show(dev, ICM20_GYRO_XOUT_H, chan->channel2, val);
        break;
    case IIO_TEMP:
        ret = icm20608_sensor_show(dev, ICM20_TEMP_OUT_H, IIO_MOD_X, val);
        break;
    default:
        return -EINVAL;
        break;
    }
    return ret;

}

static int icm20608_read_raw(struct iio_dev *indio_dev,
			  struct iio_chan_spec const *chan,
			  int *val, int *val2, long mask)
{
    struct icm20608_dev *dev = iio_priv(indio_dev); 
    unsigned char regdata = 0;
    int ret = 0;

    switch (mask){
    case IIO_CHAN_INFO_RAW:
        mutex_lock(&dev->lock);	
        ret = icm20608_read_channel_data(indio_dev, chan, val);
        mutex_unlock(&dev->lock);
        return ret;
        break;
    case IIO_CHAN_INFO_SCALE:
        switch(chan->type){
        case IIO_ACCEL:
            mutex_lock(&dev->lock);
            regdata = (icm20608_read_onereg(dev, ICM20_ACCEL_CONFIG) & 0x18) >> 3;
            *val = 0;
            *val2 = accel_scale_icm20608[regdata];
            mutex_unlock(&dev->lock);
            return IIO_VAL_INT_PLUS_NANO;
            break;
        case IIO_TEMP:
            *val = ICM20608_TEMP_SCALE/ 1000000;
			*val2 = ICM20608_TEMP_SCALE % 1000000;
			return IIO_VAL_INT_PLUS_MICRO;	/* 值为val+val2/1000000 */
            break;
        case IIO_ANGL_VEL:
            mutex_lock(&dev->lock);
            regdata = (icm20608_read_onereg(dev, ICM20_ACCEL_CONFIG) & 0x18) >> 3;
            *val = 0;
            *val2 = gyro_scale_icm20608[regdata];
            mutex_unlock(&dev->lock);
            return IIO_VAL_INT_PLUS_MICRO;
            break;
        default:
            ret = -EINVAL;
            break;
        }
        return ret;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int icm20608_write_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int val,
			   int val2,
			   long mask)
{
    return 0;
}

static const struct iio_info icm20608_info = {
	.read_raw = &icm20608_read_raw,
	.write_raw = &icm20608_write_raw,
	.driver_module = THIS_MODULE,
};

static const struct regmap_config icm20608_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
    .read_flag_mask = 0x80,
};

void icm20608_init(struct icm20608_dev *dev)
{
    regmap_write(dev->regmap, ICM20_PWR_MGMT_1, 0x80);
    mdelay(50);

    regmap_write(dev->regmap, ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);

    regmap_write(dev->regmap, ICM20_SMPLRT_DIV, 0x00);
    regmap_write(dev->regmap, ICM20_GYRO_CONFIG, 0x18);
    regmap_write(dev->regmap, ICM20_ACCEL_CONFIG, 0x18);
    regmap_write(dev->regmap, ICM20_CONFIG, 0x04);
    regmap_write(dev->regmap, ICM20_ACCEL_CONFIG2, 0x04);
    regmap_write(dev->regmap, ICM20_PWR_MGMT_2, 0x00);
    regmap_write(dev->regmap, ICM20_LP_MODE_CFG, 0x00);
    regmap_write(dev->regmap, ICM20_INT_ENABLE, 0x01);

}

static int icm20608_probe(struct spi_device *spi)
{
    struct icm20608_dev *dev;
    struct iio_dev *indio_dev;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
	if (!indio_dev)
		return -ENOMEM;

    dev = iio_priv(indio_dev);
    dev->spi = spi;
    spi_set_drvdata(spi, indio_dev);
    mutex_init(&dev->lock);


	indio_dev->channels = icm20608_channels;
	indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);
	indio_dev->dev.parent = &spi->dev;
	indio_dev->info = &icm20608_info;
    indio_dev->name = ICM20608_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;

    ret = iio_device_register(indio_dev);

    spi->mode = SPI_MODE_0;
    spi_setup(spi);
    dev->regmap = devm_regmap_init_spi(spi, &icm20608_regmap_config);

    icm20608_init(dev);

    return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
    iio_device_unregister(spi_get_drvdata(spi));
    return 0;
}

static const struct of_device_id icm20608_dt_match[] = {
	{ .compatible = "zlx,icm20608" },
	{},
};

static struct spi_driver icm20608_driver = {
	.driver = {
		.name	= "icm20608",
		.owner	= THIS_MODULE,
		.of_match_table = icm20608_dt_match,
	},
	.probe	= icm20608_probe,
	.remove	= icm20608_remove,
};

module_spi_driver(icm20608_driver);
MODULE_LICENSE("GPL");