#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>  // copy_to_user以及copy_from_user的標頭檔

MODULE_LICENSE("GPL");

// 提供一個kernel space中的緩衝區,來儲存driver的資料
// 定義16段顯示器的緩衝區,初始化為 0
static char display_buf[16] = {0};

// 根據16段顯示器的顯示資料表
// seg_for_c陣列中儲存的是無號短整數,並非字串或字元
static unsigned short seg_for_c[27] = {
    0b1111001100010001, // A
    0b0000011100000101, // b
    0b1100111100000000, // C
    0b0000011001000101, // d
    0b1000011100000001, // E
    0b1000001100000001, // F
    0b1001111100010000, // G
    0b0011001100010001, // H
    0b1100110001000100, // I
    0b1100010001000100, // J
    0b0000000001101100, // K
    0b0000111100000000, // L
    0b0011001110100000, // M
    0b0011001110001000, // N
    0b1111111100000000, // O
    0b1000001101000001, // P
    0b0111000001010000, // Q
    0b1110001100011001, // R
    0b1101110100010001, // S
    0b1100000001000100, // T
    0b0011111100000000, // U
    0b0000001100100010, // V
    0b0011001100001010, // W
    0b0000000010101010, // X
    0b0000000010100100, // Y
    0b1100110000100010, // Z
    0b0000000000000000  // 空白
};

// 將字轉換為16段顯示器可以接受的資料格式,一個長度為16的陣列,每一格不是0就是1
void char_to_binary (char c, char *sixteen_display)
{
    unsigned short binary_data = 0;

    if (c >= 'A' && c <= 'Z')
    {
        // 透過減去A字元來獲得對應的index
        binary_data = seg_for_c[c - 'A'];
    }
    else
    {
        // binary_data = seg_for_c[26];  //空白
        printk("輸入錯誤！請輸入正確格式！\n");
    }

    // 將 16 位的 binary_data 轉換為 16 位的 0 或 1 的字元
    int i;
    for(i=0; i<16; i++)
    {
        if (binary_data & (1<<i))
            sixteen_display[15-i] = '1';
        else
            sixteen_display[15-i] = '0';
    }
}

// File Operations
// read -> copy_to_user
static ssize_t display_read(struct file *fp, char *buf, size_t count, loff_t *fpos) {
    char sixteen_display[16];
    printk("call read\n");

    char_to_binary(display_buf[0],sixteen_display);

    if(copy_to_user(buf, sixteen_display, sizeof(sixteen_display))){
        printk("Failed to copy data to user\n");
        return -EFAULT;
    }

    return sizeof(sixteen_display);
}

// write -> copy from user 
static ssize_t display_write(struct file *fp,const char *buf, size_t count, loff_t *fpos)
{
    printk("call write\n");

    // 限制寫入字節數最多為 1（只接受單個字元輸入）
    if (count > 1) {
        return -EINVAL;
    }

    // 從使用者空間copy資料到內核空間
    if (copy_from_user(display_buf, buf, count)) {
        printk("Failed to copy data from user\n");
        return -EFAULT;
    }

    printk("Stored value: %c\n", display_buf[0]);

    return count;
}

// 不需要用到open
// static int my_open(struct inode *inode, struct file *fp) {
//     printk("call open\n");
//     return 0;
// }

struct file_operations my_fops = {
    read: display_read,
    write: display_write
    // 不需要用到open
    // open: my_open
};

#define MAJOR_NUM 258  // RPI中的244有被定義,改為258
#define DEVICE_NAME "my_dev"

static int my_init(void) {
    printk("call init\n");
    if(register_chrdev(MAJOR_NUM, DEVICE_NAME, &my_fops) < 0) {
        printk("Can not get major %d\n", MAJOR_NUM);
        return (-EBUSY);
    }
    printk("My device is started and the major is %d\n", MAJOR_NUM);
    return 0;
}

static void my_exit(void) {
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk("call exit\n");
}

module_init(my_init);
module_exit(my_exit);