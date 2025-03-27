#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/input/mt.h>

#define GT_CTRL_REG 	        0X8040  /* GT9147控制寄存器         */
#define GT_MODSW_REG 	        0X804D  /* GT9147模式切换寄存器        */
#define GT_9xx_CFGS_REG 	        0X8047  /* GT9147配置起始地址寄存器    */
#define GT_1xx_CFGS_REG 	        0X8050  /* GT1151配置起始地址寄存器    */
#define GT_CHECK_REG 	        0X80FF  /* GT9147校验和寄存器       */
#define GT_PID_REG 		        0X8140  /* GT9147产品ID寄存器       */

#define GT_GSTID_REG 	        0X814E  /* GT9147当前检测到的触摸情况 */
#define GT_TP1_REG 		        0X814F  /* 第一个触摸点数据地址 */
#define GT_TP2_REG 		        0X8157	/* 第二个触摸点数据地址 */
#define GT_TP3_REG 		        0X815F  /* 第三个触摸点数据地址 */
#define GT_TP4_REG 		        0X8167  /* 第四个触摸点数据地址  */
#define GT_TP5_REG 		        0X816F	/* 第五个触摸点数据地址   */
#define MAX_SUPPORT_POINTS      5       /* 最多5点电容触摸 */


struct gt9147_dev{
    int reset_pin, irq_pin;
    struct i2c_client *client;
    struct device *device;
    int max_x, max_y, irqtype;
    struct input_dev *input;
};

struct gt9147_dev gt9147;

const u8 irq_table[] = {IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING, IRQ_TYPE_LEVEL_LOW, IRQ_TYPE_LEVEL_HIGH};  /* 触发方式 */

static int gt9147_read_regs(struct gt9147_dev *dev, u16 reg, u8 *buf, int len)
{
    int ret;
    struct i2c_msg msg[2];
    u8 regdata[2];
    struct i2c_client *client = (struct i2c_client *)dev->client;

    regdata[0] = reg >> 8;
    regdata[1] = reg & 0xff;

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &regdata[0];
    msg[0].len = 2;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = buf;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2) {
		ret = 0;
	} else {
		ret = -EREMOTEIO;
	}
	return ret;
}

static int gt9147_write_regs(struct gt9147_dev *dev, u16 reg, u8 *buf, int len)
{
    int ret;
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->client;
    u8 b[256];

    b[0] = reg >> 8;
    b[1] = reg & 0xff;
    memcpy(&b[2], buf, len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 2;

    ret = i2c_transfer(client->adapter, &msg, 1);
	return ret;
}


static int gt9147_ts_reset(struct i2c_client *client, struct gt9147_dev *dev)
{
    int ret;
    ret = devm_gpio_request_one(&client->dev, dev->reset_pin, GPIOF_OUT_INIT_HIGH, "gt9147 reset");
    if (ret) {
			return ret;
		}

    ret = devm_gpio_request_one(&client->dev, dev->irq_pin, GPIOF_OUT_INIT_HIGH, "gt9147 irq");
    if (ret) {
			return ret;
		}

    /* 4、初始化GT9147，要严格按照GT9147时序要求 */
    gpio_set_value(dev->reset_pin, 0); /* 复位GT9147 */
    msleep(10);
    gpio_set_value(dev->reset_pin, 1); /* 停止复位GT9147 */
    msleep(10);
    gpio_set_value(dev->irq_pin, 0);    /* 拉低INT引脚 */
    msleep(50);
    gpio_direction_input(dev->irq_pin); /* INT引脚设置为输入 */

    return 0;
}

static int gt9147_read_firmware(struct i2c_client *client, struct gt9147_dev *dev)
{
    int ret;
    u8 data[7];

    ret = gt9147_read_regs(dev, GT_9xx_CFGS_REG, data, 7);
    if (ret) {
		dev_err(&client->dev, "Unable to read Firmware.\n");
		return ret;
	}

    dev->max_x = (data[2] << 8) | data[1];
    dev->max_y = (data[4] << 8) | data[3];
    dev->irqtype = data[6] & 0x3;

    return 0;
}

static irqreturn_t gt9147_irq_handler(int irq, void *dev_id)
{
    int ret, touch_num = 0;
    int input_x, input_y;
    int id = 0;
    u8 data;
    u8 touch_data[5];

    struct gt9147_dev *dev = dev_id;
    
    ret = gt9147_read_regs(dev, GT_GSTID_REG, &data, 1);
    if(data == 0){
        goto fail;
    }else{
        touch_num = data & 0x0f;
    }

    if(touch_num){
        gt9147_read_regs(dev, GT_TP1_REG, touch_data, 5);
        id = touch_data[0] & 0x0F;
        if(id == 0) {
            input_x  = touch_data[1] | (touch_data[2] << 8);
            input_y  = touch_data[3] | (touch_data[4] << 8);

            input_mt_slot(dev->input, id);
		    input_mt_report_slot_state(dev->input, MT_TOOL_FINGER, true);
		    input_report_abs(dev->input, ABS_MT_POSITION_X, input_x);
		    input_report_abs(dev->input, ABS_MT_POSITION_Y, input_y);

        }
    } else if(touch_num == 0){                /* 单点触摸释放 */
        input_mt_slot(dev->input, id);
        input_mt_report_slot_state(dev->input, MT_TOOL_FINGER, false);
    }

    input_mt_report_pointer_emulation(dev->input, true);
    input_sync(dev->input);

    data = 0x00;                /* 向0X814E寄存器写0 */
    gt9147_write_regs(dev, GT_GSTID_REG, &data, 1);

    
    return 0;
fail:
	return IRQ_HANDLED;
}

static int gt9147_irq_init(struct gt9147_dev *dev, struct i2c_client *client)
{
    int ret;
    ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, gt9147_irq_handler, 
                                    irq_table[dev->irqtype] | IRQF_ONESHOT,
					                    client->name, &gt9147);

    if (ret) {
		dev_err(&client->dev, "Unable to request touchscreen IRQ.\n");
		return ret;
	}
    return 0;
}

static int gt9147_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    u8 data, ret;
    gt9147.client = client;

    gt9147.reset_pin = of_get_named_gpio(client->dev.of_node, "reset-gpios", 0);
    gt9147.irq_pin = of_get_named_gpio(client->dev.of_node, "interrupt-gpios", 0);

    gt9147_ts_reset(client, &gt9147);

    data = 0xAA;
    gt9147_write_regs(&gt9147, GT_CTRL_REG, &data, 1);
    mdelay(100);
    data = 0x0;
    gt9147_write_regs(&gt9147, GT_CTRL_REG, &data, 1); 
    mdelay(100);

    ret = gt9147_read_firmware(client, &gt9147);
    if(ret != 0) {
		printk("Fail !!! check !!\r\n");
		goto fail;
    }

    gt9147.input = devm_input_allocate_device(&client->dev);
    gt9147.input->name = client->name;
    gt9147.input->id.bustype = BUS_I2C;
    gt9147.input->dev.parent = &client->dev;

    __set_bit(EV_KEY, gt9147.input->evbit);
	__set_bit(EV_ABS, gt9147.input->evbit);
	__set_bit(BTN_TOUCH, gt9147.input->keybit);

    input_set_abs_params(gt9147.input, ABS_X, 0, gt9147.max_x, 0, 0);
	input_set_abs_params(gt9147.input, ABS_Y, 0, gt9147.max_y, 0, 0);
	input_set_abs_params(gt9147.input, ABS_MT_POSITION_X,0, gt9147.max_x, 0, 0);
	input_set_abs_params(gt9147.input, ABS_MT_POSITION_Y,0, gt9147.max_y, 0, 0);	
    ret = input_mt_init_slots(gt9147.input, MAX_SUPPORT_POINTS, 0);
	if (ret) {
		goto fail;
	}

    ret = input_register_device(gt9147.input);
    if (ret)
		goto fail;

    ret = gt9147_irq_init(&gt9147, client);
	if(ret < 0) {
		goto fail;
	} 
    
    return 0;
fail:
	return ret;
}

static int gt9147_remove(struct i2c_client *client)
{
    input_unregister_device(gt9147.input);
    return 0;
}

static const struct i2c_device_id gt9147_ids[] = {
	{ "goodix,gt9147" },
	{},
};

static const struct of_device_id gt9147_of_match_table[] = {
	{ .compatible = "goodix,gt9147", },
	{ },
};

static struct i2c_driver gt9147_driver = {
	.driver = {
		.name = "gt9147",
		.owner = THIS_MODULE,
		.of_match_table = gt9147_of_match_table,
	},
	.probe = gt9147_probe,
	.remove	= gt9147_remove,
	.id_table = gt9147_ids,
};

module_i2c_driver(gt9147_driver);
MODULE_LICENSE("GPL");
