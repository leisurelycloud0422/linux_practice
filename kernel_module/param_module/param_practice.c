#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int number = 42;
static char *name = "World";
static bool enable_debug = false;

module_param(number, int, 0644);
MODULE_PARM_DESC(number, "輸入一個整數");

module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "輸入一個字串");

module_param(enable_debug, bool, 0644);
MODULE_PARM_DESC(enable_debug, "啟用 debug 模式");

static int __init param_init(void)
{
    pr_info("param_practice: 模組載入\n");
    pr_info("param_practice: number = %d\n", number);
    pr_info("param_practice: name = %s\n", name);

    if (enable_debug)
        pr_info("param_practice: Debug 模式已啟用\n");
    else
        pr_info("param_practice: Debug 模式未啟用\n");

    return 0;
}

static void __exit param_exit(void)
{
    pr_info("param_practice: 模組卸載\n");
}

module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你");
MODULE_DESCRIPTION("練習 module_param 的範例");
