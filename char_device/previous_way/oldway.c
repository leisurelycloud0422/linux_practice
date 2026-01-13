#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE];
static int buffer_size = 0;

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
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("Failed to register char device\n");
        return major;
    }
    pr_info("mydev loaded with major number %d\n", major);
    return 0;
}

static void __exit mydev_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("mydev unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你的名字");
MODULE_DESCRIPTION("A simple char device supporting echo/cat");
