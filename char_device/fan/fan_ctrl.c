#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/i2c.h>    /* 必須包含 I2C 標頭檔 */

#define DEVICE_NAME "fan_ctrl"
#define BUFFER_SIZE 64

static dev_t dev_num;
static struct cdev fan_cdev;
static struct class *fan_class;
static struct device *fan_device;

/* 定義全域 fan_client 指針 */
static struct i2c_client *fan_client = NULL; 

static char fan_buffer[BUFFER_SIZE];
static size_t buffer_size = 0;

static DEFINE_MUTEX(fan_mutex);
static int fan_open_count = 0; 

/*
 * 模擬硬體動作
 */
static int fan_hw_init(void)
{
    int ret;

    /* 安全檢查：如果沒有實際綁定 I2C 設備，先跳過硬體操作以免崩潰 */
    if (!fan_client) {
        pr_warn("fan_ctrl: [HW] No I2C client bound, skipping HW init\n");
        return -ENODEV;
    }

    pr_info("fan_ctrl: [HW] Initializing fan controller (MAX31790)\n");

    ret = i2c_smbus_write_byte_data(fan_client, 0x00, 0x81);
    if (ret < 0) return ret;

    ret = i2c_smbus_write_byte_data(fan_client, 0x30, 128);
    if (ret < 0) return ret;

    pr_info("fan_ctrl: [HW] Fan controller initialized successfully\n");
    return 0;
}

static int fan_hw_shutdown(void)
{
    if (!fan_client) return -ENODEV;

    pr_info("fan_ctrl: [HW] Shutting down fan controller\n");
    i2c_smbus_write_byte_data(fan_client, 0x30, 0x00);
    return 0;
}

/*
 * file_operations
 */
static int fan_open(struct inode *inode, struct file *file)
{
    mutex_lock(&fan_mutex);
    fan_open_count++;
    pr_info("fan_ctrl: open (%d users)\n", fan_open_count);

    if (fan_open_count == 1) {
        fan_hw_init();
    }
    mutex_unlock(&fan_mutex);
    return 0;
}

static int fan_release(struct inode *inode, struct file *file)
{
    mutex_lock(&fan_mutex);
    fan_open_count--;
    pr_info("fan_ctrl: release (%d users left)\n", fan_open_count);

    if (fan_open_count == 0) {
        fan_hw_shutdown();
    }
    mutex_unlock(&fan_mutex);
    return 0;
}

static ssize_t fan_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
    ssize_t ret;
    mutex_lock(&fan_mutex);
    if (*ppos >= buffer_size) {
        ret = 0;
        goto out;
    }
    if (count > buffer_size - *ppos)
        count = buffer_size - *ppos;
    if (copy_to_user(buf, fan_buffer + *ppos, count)) {
        ret = -EFAULT;
        goto out;
    }
    *ppos += count;
    ret = count;
out:
    mutex_unlock(&fan_mutex);
    return ret;
}

static ssize_t fan_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    ssize_t ret;
    if (count > BUFFER_SIZE)
        count = BUFFER_SIZE;
    mutex_lock(&fan_mutex);
    if (copy_from_user(fan_buffer, buf, count)) {
        ret = -EFAULT;
        goto out;
    }
    buffer_size = count;
    pr_info("fan_ctrl: command received: '%.*s'\n", (int)count, fan_buffer);
    ret = count;
out:
    mutex_unlock(&fan_mutex);
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fan_open,
    .release = fan_release,
    .read = fan_read,
    .write = fan_write,
};

static int __init fan_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret) return ret;

    cdev_init(&fan_cdev, &fops);
    fan_cdev.owner = THIS_MODULE;

    ret = cdev_add(&fan_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    /* 修正點：6.4 之後的核心版本只需一個參數 */
    fan_class = class_create(DEVICE_NAME);
    if (IS_ERR(fan_class)) {
        cdev_del(&fan_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(fan_class);
    }

    fan_device = device_create(fan_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(fan_device)) {
        class_destroy(fan_class);
        cdev_del(&fan_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(fan_device);
    }

    pr_info("fan_ctrl loaded: major=%d minor=%d\n", MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit fan_exit(void)
{
    device_destroy(fan_class, dev_num);
    class_destroy(fan_class);
    cdev_del(&fan_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("fan_ctrl unloaded\n");
}

module_init(fan_init);
module_exit(fan_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Enhanced BMC Fan Control char device driver");