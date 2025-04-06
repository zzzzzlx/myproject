#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/kernel.h>



struct led_beep_dev{
    struct led_classdev cdev;
    struct gpio_desc *gpio;
};

void dev_led_set(struct led_classdev *led_cdev, enum led_brightness value)
{
    struct led_beep_dev *priv = container_of(led_cdev, struct led_beep_dev, cdev);

    gpiod_set_value(priv->gpio, value);
}


static int led_beep_probe(struct platform_device *pdev)
{
    struct led_beep_dev *dev;
    char a[20];
    const char *my_name = a;
    struct device_node *np = pdev->dev.of_node;


    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    of_property_read_string(np, "my_name", &my_name);

    dev->gpio = devm_gpiod_get(&pdev->dev, my_name, GPIOD_OUT_LOW);

    dev->cdev.name = my_name;
    dev->cdev.brightness = 0;
    dev->cdev.brightness_set = dev_led_set;

    devm_led_classdev_register(&pdev->dev, &dev->cdev);

    platform_set_drvdata(pdev, dev);
    printk("my_probe run, my_name = %s\n", my_name);
    return 0;
}

static struct of_device_id my_dev_match[] = {
	{.compatible = "zlx,gpioled"}, 
	{.compatible = "zlx,beep"}, 
	{},
};

static struct platform_driver led_beep_driver = {
	.probe = led_beep_probe,
	.driver = {
		.name = "led_beep",
		.of_match_table = my_dev_match,
	},
};

module_platform_driver(led_beep_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zlx");