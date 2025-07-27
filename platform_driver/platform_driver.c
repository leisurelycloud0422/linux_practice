#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

static int my_probe(struct platform_device *pdev)
{
    pr_info("my_platform_driver: probe called for %s\n", pdev->name);
    return 0;
}

static void my_remove(struct platform_device *pdev)
{
    pr_info("my_platform_driver: remove called\n");
}

static const struct of_device_id my_of_ids[] = {
    { .compatible = "mydev,mychip" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_platform_driver = {
    .probe  = my_probe,
    .remove = my_remove,
    .driver = {
        .name           = "my_platform_driver",
        .of_match_table = my_of_ids,
    },
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
