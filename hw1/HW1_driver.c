/* hw1_driver_module.c - Kernel module 作為裝置接收距離與金額，控制七段顯示器與 LED */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/delay.h>

#define DEVICE_NAME "HW1_dev"
#define CLASS_NAME  "HW1_class"

static dev_t dev_num;
static struct cdev cdev;
static struct class *cls;

/* LED GPIO 假設使用 GPIO17~GPIO24 */
#define MAX_LED 8
static int led_base = 17;

/* 七段 GPIO: a~g 共7段 */
static int seg_pins[7] = {5, 6, 7, 8, 9, 10, 11};

/* 共陽七段字元顯示表 */
static const uint8_t seg_table[128] = {
	['0'] = 0b00111111,
	['1'] = 0b00000110,
	['2'] = 0b01011011,
	['3'] = 0b01001111,
	['4'] = 0b01100110,
	['5'] = 0b01101101,
	['6'] = 0b01111101,
	['7'] = 0b00000111,
	['8'] = 0b01111111,
	['9'] = 0b01101111,
};

static void init_gpio(void)
{
	int i;

	for (i = 0; i < MAX_LED; i++) {
		int pin = led_base + i;

		if (gpio_request(pin, "led") == 0) {
			gpio_direction_output(pin, 0);
			pr_info("LED GPIO %d initialized\n", pin);
		} else {
			pr_err("LED GPIO %d request failed\n", pin);
		}
	}

	for (i = 0; i < 7; i++) {
		if (gpio_request(seg_pins[i], "seg") == 0) {
			gpio_direction_output(seg_pins[i], 0);
			pr_info("SEG GPIO %d initialized\n", seg_pins[i]);
		} else {
			pr_err("SEG GPIO %d request failed\n", seg_pins[i]);
		}
	}
}

static void cleanup_gpio(void)
{
	int i;

	for (i = 0; i < MAX_LED; i++) {
		gpio_set_value(led_base + i, 0);
		gpio_free(led_base + i);
	}
	for (i = 0; i < 7; i++) {
		gpio_set_value(seg_pins[i], 0);
		gpio_free(seg_pins[i]);
	}
}

static void show_led_distance(int km)
{
	int i;

	pr_info("Display distance on LED: %d\n", km);
	if (km > MAX_LED)
		km = MAX_LED;

	for (i = 0; i < km; i++)
		gpio_set_value(led_base + i, 1);

	for (i = 0; i < km; i++) {
		msleep(1000);
		gpio_set_value(led_base + i, 0);
	}
}

static void show_price_on_7seg(int price)
{
	char str[8];
	int i;

	pr_info("Display price on 7seg: %d\n", price);
	sprintf(str, "%d", price);

	for (i = 0; str[i] != '\0'; i++) {
		char ch = str[i];
		uint8_t pattern = seg_table[(int)ch];
		int j;

		for (j = 0; j < 7; j++)
			gpio_set_value(seg_pins[j], (pattern >> j) & 1);

		msleep(1000);
	}
}

static int dev_open(struct inode *inode, struct file *file)
{
	pr_info("hw1_driver: device opened\n");
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	pr_info("hw1_driver: device closed\n");
	return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
			 size_t len, loff_t *off)
{
	char kbuf[32];
	int distance = 0, price = 0;

	if (len >= sizeof(kbuf))
		len = sizeof(kbuf) - 1;

	if (copy_from_user(kbuf, buf, len))
		return -EFAULT;

	kbuf[len] = '\0';

	if (sscanf(kbuf, "%d,%d", &distance, &price) == 2) {
		pr_info("hw1_driver: get distance %d km, get total %d\n",
			distance, price);
		pr_info("kbuf raw: %s\n", kbuf);
		show_led_distance(distance);
		show_price_on_7seg(price);
	} else {
		pr_err("hw1_driver: 格式錯誤，預期 <距離>,<金額>\n");
	}
	return len;
}

static const struct file_operations fops = {
	.owner   = THIS_MODULE,
	.open    = dev_open,
	.release = dev_release,
	.write   = dev_write,
};

static int __init hw1_init(void)
{
	alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	cdev_init(&cdev, &fops);
	cdev_add(&cdev, dev_num, 1);

	cls = class_create(THIS_MODULE, CLASS_NAME);
	device_create(cls, NULL, dev_num, NULL, DEVICE_NAME);

	init_gpio();

	pr_info("hw1_driver: 模組已載入\n");
	return 0;
}

static void __exit hw1_exit(void)
{
	cleanup_gpio();
	device_destroy(cls, dev_num);
	class_destroy(cls);
	cdev_del(&cdev);
	unregister_chrdev_region(dev_num, 1);
	pr_info("hw1_driver: 模組已卸載\n");
}

module_init(hw1_init);
module_exit(hw1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("user");
MODULE_DESCRIPTION("HW1 Kernel Driver Module 控制七段顯示器與 LED");

