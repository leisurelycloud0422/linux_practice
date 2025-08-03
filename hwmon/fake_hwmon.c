#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>

static int fake_temp_read(struct device *dev, enum hwmon_sensor_types type,
                          u32 attr, int channel, long *val)
{
    static int fake_temp = 30000; // 單位：毫度C（30°C）
    fake_temp += 100; // 每次讀取 +0.1°C
    if (fake_temp > 80000) fake_temp = 30000; // 循環
    *val = fake_temp;
    return 0;
}

static umode_t fake_is_visible(const void *data,
                               enum hwmon_sensor_types type,
                               u32 attr, int channel)
{
    if (type == hwmon_temp && attr == hwmon_temp_input)
        return 0444; // 可讀
    return 0;
}

static const struct hwmon_ops fake_hwmon_ops = {
    .is_visible = fake_is_visible,
    .read = fake_temp_read,
};

static const struct hwmon_channel_info *fake_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
    NULL,
};

static const struct hwmon_chip_info fake_chip_info = {
    .ops = &fake_hwmon_ops,
    .info = fake_info,
};

static int fake_probe(struct platform_device *pdev)
{
    struct device *hwmon_dev;

    hwmon_dev = hwmon_device_register_with_info(&pdev->dev, "fake_temp_sensor",
                                                NULL, &fake_chip_info, NULL);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static void fake_remove(struct platform_device *pdev)
{
    hwmon_device_unregister(&pdev->dev);
}

static struct platform_driver fake_driver = {
    .driver = {
        .name = "fake_temp_driver",
    },
    .probe = fake_probe,
    .remove = fake_remove,
};

static void __exit fake_exit(void)
{
    platform_driver_unregister(&fake_driver);
}

static int __init fake_init(void)
{
    struct platform_device *pdev;

    platform_driver_register(&fake_driver);
    pdev = platform_device_register_simple("fake_temp_driver", -1, NULL, 0);
    return PTR_ERR_OR_ZERO(pdev);
}

module_init(fake_init);
module_exit(fake_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你");
MODULE_DESCRIPTION("Fake HWMON Temperature Driver");
