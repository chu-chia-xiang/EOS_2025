#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/types.h>

#define DEVICE_NAME "lab3_dev"
#define CLASS_NAME  "lab3_class"

static dev_t dev_num;
static struct cdev lab3_cdev;
static struct class *lab3_class;

static char id_buffer[128] = {0};   // 學號儲存
static int idx = 0;                 // 跑馬燈目前位置
static int id_length = 0;           // 學號長度

// 七段顯示器 GPIO 腳位：a ~ g（依你的配線調整）
static int seg_pins[7] = {17, 18, 27, 22, 23, 24, 25};

// 七段字元對應 bit pattern（bit0→a, bit1→b, ..., bit6→g）
// 註：此表對應「1=亮」的極性（常見為共陰）。若為共陽，輸出時取反。
static const u8 seg_table[128] = {
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
	['E']=0b01111001 
};

static void init_seg_gpio(void)
{
	int i;
	for (i = 0; i < 7; i++) {
		gpio_request(seg_pins[i], "seg");
		gpio_direction_output(seg_pins[i], 0);
	}
}

static void cleanup_seg_gpio(void)
{
	int i;
	for (i = 0; i < 7; i++) {
		gpio_set_value(seg_pins[i], 0);
		gpio_free(seg_pins[i]);
	}
}

static void display_char_on_7seg(char ch)
{
	u8 pattern = seg_table[(int)ch];
	int i;
	for (i = 0; i < 7; i++) {
		/* 共陰：直接輸出；若共陽改成：!((pattern >> i) & 1) */
		gpio_set_value(seg_pins[i], (pattern >> i) & 1);
	}
}

static int lab3_open(struct inode *inode, struct file *file)
{
	pr_info("lab3_driver: Device opened\n");
	return 0;
}

static int lab3_release(struct inode *inode, struct file *file)
{
	pr_info("lab3_driver: Device closed\n");
	return 0;
}

static ssize_t lab3_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *off)
{
	if (len > sizeof(id_buffer) - 1)
		len = sizeof(id_buffer) - 1;

	if (copy_from_user(id_buffer, buf, len))
		return -EFAULT;

	id_buffer[len] = '\0';
	id_length = (int)len;
	idx = 0;

	pr_info("lab3_driver: Stored student ID: %s\n", id_buffer);
	return len;
}

static ssize_t lab3_read(struct file *file, char __user *buf,
                         size_t len, loff_t *off)
{
	char ch;

	if (id_length == 0)
		return 0;

	ch = id_buffer[idx];
	display_char_on_7seg(ch);

	if (copy_to_user(buf, &ch, 1))
		return -EFAULT;

	idx = (idx + 1) % id_length;
	return 1;
}

static const struct file_operations fops = {
	.owner   = THIS_MODULE,
	.open    = lab3_open,
	.release = lab3_release,
	.write   = lab3_write,
	.read    = lab3_read,
};

static int __init lab3_init(void)
{
	alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	cdev_init(&lab3_cdev, &fops);
	cdev_add(&lab3_cdev, dev_num, 1);

	lab3_class = class_create(THIS_MODULE, CLASS_NAME);
	device_create(lab3_class, NULL, dev_num, NULL, DEVICE_NAME);

	init_seg_gpio();

	pr_info("lab3_driver: Module loaded\n");
	return 0;
}

static void __exit lab3_exit(void)
{
	cleanup_seg_gpio();

	device_destroy(lab3_class, dev_num);
	class_destroy(lab3_class);
	cdev_del(&lab3_cdev);
	unregister_chrdev_region(dev_num, 1);

	pr_info("lab3_driver: Module removed\n");
}

module_init(lab3_init);
module_exit(lab3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Wang");
MODULE_DESCRIPTION("Lab3 seven segment display driver");

