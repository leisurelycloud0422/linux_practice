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

static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *ppos)
{
    if (*ppos >= buffer_size)
        return 0;
    if (count > buffer_size - *ppos)
        count = buffer_size - *ppos;

    if (copy_to_user(buf, device_buffer + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
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

    // 4. 建立 class
    mydev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(mydev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to create class\n");
        return PTR_ERR(mydev_class);
    }

    // 5. 建立 device（會自動在 /dev/mydev 出現）
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
