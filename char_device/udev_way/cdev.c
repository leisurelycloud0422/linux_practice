#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 1024

static dev_t dev_num;              // major + minor
static struct cdev my_cdev;        // cdev structure
static struct class *mydev_class;
static struct device *mydev_device;

static char device_buffer[BUFFER_SIZE];
static int buffer_size = 0;
static char kernel_buffer[1024];
static int pos = 0;

// ---- File operations ----
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mydev: opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mydev: closed\n");
    return 0;
}

static ssize_t mydev_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos)
{
    if (count > sizeof(kernel_buffer) - 1)
        count = sizeof(kernel_buffer) - 1;

    if (copy_from_user(kernel_buffer, buf, count))
        return -EFAULT;

    kernel_buffer[count] = '\0';
    pos = count;
    printk(KERN_INFO "mydev: write %s\n", kernel_buffer);

    return count;
}

static ssize_t mydev_read(struct file *file, char __user *buf,
                          size_t count, loff_t *ppos)
{
    if (*ppos >= pos)
        return 0;

    if (count > pos - *ppos)
        count = pos - *ppos;

    if (copy_to_user(buf, kernel_buffer + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = mydev_read,
    .write = mydev_write,
};

// ---- Module init ----
static int __init mydev_init(void)
{
    int ret;

    // 1. 動態申請 major/minor（現代方式）
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate chrdev region\n");
        return ret;
    }
    pr_info("Allocated Major=%d Minor=%d\n", MAJOR(dev_num), MINOR(dev_num));

    // 2. 初始化 cdev
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // 3. 加入 cdev 到核心
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to add cdev\n");
        return ret;
    }

    // ---- 建立 class ----
    mydev_class = class_create(DEVICE_NAME);
    if (IS_ERR(mydev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to create class\n");
        return PTR_ERR(mydev_class);
    }

// ---- 建立 device ----
    mydev_device = device_create(mydev_class, NULL, dev_num,
                                NULL, DEVICE_NAME);
    if (IS_ERR(mydev_device)) {
        class_destroy(mydev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to create device\n");
        return PTR_ERR(mydev_device);
    }

    pr_info("mydev loaded successfully\n");
    return 0;
}

// ---- Module exit ----
static void __exit mydev_exit(void)
{
    device_destroy(mydev_class, dev_num);
    class_destroy(mydev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("mydev unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你的名字");
MODULE_DESCRIPTION("Modern char device driver using cdev + class + device_create");
