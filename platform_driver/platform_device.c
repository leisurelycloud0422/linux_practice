#include <linux/module.h>
#include <linux/platform_device.h>

static void my_device_release(struct device *dev)
{
    pr_info("my_platform_device: device.release() called\n");
}

static struct platform_device my_fake_device = {
    .name = "my_platform_driver",
    .id = -1,
    // 2. 在這裡綁定它
    .dev = {
        .release = my_device_release,
    },
};
static int __init my_device_init(void)
{
    return platform_device_register(&my_fake_device);
}

static void __exit my_device_exit(void)
{
    pr_info("my_platform_device: unregistering device\n");
    platform_device_unregister(&my_fake_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");

