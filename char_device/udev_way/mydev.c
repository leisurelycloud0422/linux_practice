#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>  // class_create, device_create

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE];
static int buffer_size = 0;

static struct class *mydev_class;
static struct device *mydev_device;

static int my_open(struct inode *inode, struct file *file) {
    pr_info("mydev: opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    pr_info("mydev: closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    if (*ppos >= buffer_size)
        return 0;
    if (count > buffer_size - *ppos)
        count = buffer_size - *ppos;

    if (copy_to_user(buf, device_buffer + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    if (count > BUFFER_SIZE)
        count = BUFFER_SIZE;

    if (copy_from_user(device_buffer, buf, count))
        return -EFAULT;

    buffer_size = count;
    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init mydev_init(void) {
    // 1. 註冊字元裝置
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("Failed to register char device\n");
        return major;
    }

    // 2. 建立 class（出現在 /sys/class/mydev）
    mydev_class = class_create(DEVICE_NAME);
    if (IS_ERR(mydev_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        pr_err("Failed to create class\n");
        return PTR_ERR(mydev_class);
    }

    // 3. 建立 device（自動在 /dev/mydev 出現）
    mydev_device = device_create(mydev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mydev_device)) {
        class_destroy(mydev_class);
        unregister_chrdev(major, DEVICE_NAME);
        pr_err("Failed to create device\n");
        return PTR_ERR(mydev_device);
    }

    pr_info("mydev loaded with major number %d\n", major);
    return 0;
}

static void __exit mydev_exit(void) {
    device_destroy(mydev_class, MKDEV(major, 0));
    class_destroy(mydev_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("mydev unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你的名字");
MODULE_DESCRIPTION("A simple char device with class + udev");
