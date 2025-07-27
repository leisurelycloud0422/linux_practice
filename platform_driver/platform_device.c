#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device my_fake_device = {
    .name = "my_platform_driver",  // 要與 driver 中 .driver.name 相同
    .id = -1,
};

static int __init my_device_init(void)
{
    return platform_device_register(&my_fake_device);
}

static void __exit my_device_exit(void)
{
    platform_device_unregister(&my_fake_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");

