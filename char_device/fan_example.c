#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#define DEVICE_NAME "fan_ctrl"
#define BUFFER_SIZE 64

static dev_t dev_num;
static struct cdev fan_cdev;
static struct class *fan_class;
static struct device *fan_device;

static char fan_buffer[BUFFER_SIZE];
static size_t buffer_size = 0;

static DEFINE_MUTEX(fan_mutex);
static int fan_open_count = 0; // reference count

/*
 * 模擬硬體動作
 */
static int fan_hw_init(void)
{
    int ret;

    pr_info("fan_ctrl: [HW] Initializing fan controller (MAX31790)\n");

    /* Step 1: 設定 FAN_CONFIG1_0 (0x00)
     * Bit 7: PWM mode enable
     * Bit 0: Tach enable
     */
    ret = i2c_smbus_write_byte_data(fan_client,
                                    0x00,   // FAN_CONFIG1_0
                                    0x81);  // PWM enable + Tach enable
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to configure FAN_CONFIG1_0\n");
        return ret;
    }

    /* Step 2: 設定 PWM duty cycle，例如 50% */
    ret = i2c_smbus_write_byte_data(fan_client,
                                    0x30,   // FAN_PWM_0
                                    128);   // 50% duty (0~255)
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to set PWM duty\n");
        return ret;
    }

    /* Step 3: 設定 Tach target，例如 4000 RPM (假設換算已經處理過) */
    ret = i2c_smbus_write_word_data(fan_client,
                                    0x40,   // FAN_TACH_TARGET_LSB
                                    0x1388); // 0x1388 = 5000 target
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to set Tach target\n");
        return ret;
    }

    /* Step 4: 清除錯誤狀態 */
    ret = i2c_smbus_write_byte_data(fan_client,
                                    0x90,   // FAN_FAULT_STATUS
                                    0xFF);  // clear faults
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to clear fault status\n");
        return ret;
    }

    pr_info("fan_ctrl: [HW] Fan controller initialized successfully\n");

    return 0;
}
static int fan_hw_shutdown(void)
{
    int ret;

    pr_info("fan_ctrl: [HW] Shutting down fan controller\n");

    /* 1. 將 PWM duty 設為 0%，等於停轉 */
    ret = i2c_smbus_write_byte_data(fan_client,
                                    0x30,   // FAN_PWM_0
                                    0x00);  // 0% PWM
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to stop PWM\n");
        return ret;
    }

    /* 2. 關閉 PWM + Tach */
    ret = i2c_smbus_write_byte_data(fan_client,
                                    0x00,   // FAN_CONFIG1_0
                                    0x00);  // disable all
    if (ret < 0) {
        pr_err("fan_ctrl: Failed to disable fan controller\n");
        return ret;
    }

    pr_info("fan_ctrl: [HW] Fan hardware shutdown completed\n");

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
        // 第一次被開啟 → 初始化硬體
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
        // 最後一個 user 離開 → 關閉硬體
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

    // 你可以在這裡解析 PWM 或 I2C 寄存器指令

    ret = count;

out:
    mutex_unlock(&fan_mutex);
    return ret;
}

/*
 * file_operations 連結
 */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fan_open,
    .release = fan_release,
    .read = fan_read,
    .write = fan_write,
};

/*
 * Module init / exit
 */
static int __init fan_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("fan_ctrl: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&fan_cdev, &fops);
    fan_cdev.owner = THIS_MODULE;

    ret = cdev_add(&fan_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("fan_ctrl: cdev_add failed\n");
        return ret;
    }

    fan_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(fan_class)) {
        cdev_del(&fan_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("fan_ctrl: class_create failed\n");
        return PTR_ERR(fan_class);
    }

    fan_device = device_create(fan_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(fan_device)) {
        class_destroy(fan_class);
        cdev_del(&fan_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("fan_ctrl: device_create failed\n");
        return PTR_ERR(fan_device);
    }

    pr_info("fan_ctrl loaded: major=%d minor=%d\n",
             MAJOR(dev_num), MINOR(dev_num));
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
