#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/string.h>

int major;
int dev_cnt;
static struct class *my_dev_class;
char dev_names[10][20]={};    //保存设备树中的名字，用于分辨设备
struct gpio_desc *my_dev_gpio[10];  //保存GPIO描述符

static ssize_t my_drv_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
	struct inode *inode = file_inode(filp);
	int minor = iminor(inode);
	char status;
	int err;

	status = gpiod_get_value(my_dev_gpio[minor]);  /* 物理电平值 */
	err = copy_to_user(buf, &status, 1);
    return 0;
}

static ssize_t my_drv_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
	struct inode *inode = file_inode(filp);
	int minor =iminor(inode);
	char status;
	int err;
	err = copy_from_user(&status, buf, sizeof(status));

	gpiod_set_value(my_dev_gpio[minor], status);

    return 0;
}

static int my_drv_open (struct inode *node, struct file *filp)
{
    return 0;
}

static int my_drv_release (struct inode *node, struct file *filp)
{
    return 0;
}

static struct file_operations my_dev_ops = {
	.owner		= 	THIS_MODULE,
	.read 		=	my_drv_read,
	.write		=	my_drv_write,
	.open		=	my_drv_open,
	.release	=	my_drv_release,
};

static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;

    //	int gpio_pin;
	char a[20];
	const char *my_name = a;

    of_property_read_string(np, "my_name", &my_name);
    strcpy(dev_names[dev_cnt], my_name);


	//从struct desc结构体转成GPIO子系统标号
	//	gpio_pin = desc_to_gpio(my_dev_gpio[dev_cnt]);
	//	gpio_s[dev_cnt] = gpio_pin;   //保存GPIO号
    my_dev_gpio[dev_cnt] = gpiod_get(dev, my_name, GPIOD_OUT_LOW);
    gpio_direction_output(desc_to_gpio(my_dev_gpio[dev_cnt]), 1);

	//创建设备节点 /dev/xxx
    device_create(my_dev_class, NULL, MKDEV(major, dev_cnt), NULL, my_name);
    dev_cnt++;

    printk("my_probe run, my_name = %s\n", my_name);

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

    int gpio_pin, i;
    char a[20];
	const char *my_name = a;

    /* 通过my_name  属性匹配设备 */
	of_property_read_string(np, "my_name", &my_name);
	for(i = 0; i < dev_cnt; i++){
		if(strcmp(dev_names[i],my_name) == 0){
			strcpy(dev_names[i], "");
			gpio_pin = desc_to_gpio(my_dev_gpio[i]); 
			//释放GPIO
			gpiod_put(my_dev_gpio[i]);
			//销毁设备
			device_destroy(my_dev_class, MKDEV(major, i));
			printk("my_remove run, device_destroy %s, my_gpio = %d\n", my_name, gpio_pin);
		}
	}

    return 0;
}

static struct of_device_id my_dev_match[] = {
	{.compatible = "zlx,gpioled"}, 
	{.compatible = "zlx,beep"}, 
	{},
};

static struct platform_driver dev_driver = {
	.probe		=	my_probe,	
	.remove		= 	my_remove,
	.driver		= {
		.name	= "my_platform_driver",
		.of_match_table = my_dev_match,
	},
};

static int dev_init(void)
{
    major = register_chrdev(0, "hc_dev_drv", &my_dev_ops);
    if(major < 0){
		printk("register_chrdev famy\n");
		return major;
	}

    my_dev_class = class_create(THIS_MODULE, "my_dev_class");
    if(IS_ERR(my_dev_class)){
		printk("class_create failed\n");
		return 1;
	}

    platform_driver_register(&dev_driver);

    return 0;
}

static void dev_exit(void)
{
	platform_driver_unregister(&dev_driver);
	class_destroy(my_dev_class);
	unregister_chrdev(major, "hc_dev_drv");
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
