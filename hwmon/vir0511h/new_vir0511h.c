#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#define MAX_TEMP_NUM 4

#define VIR0511H_CONVERSION_REG     0x00
#define VIR0511H_CONFIG_REG         0x01

/* 模擬不同通道的設定值 */
static uint16_t def_config_data[MAX_TEMP_NUM] = {0x0001, 0x0002, 0x0004, 0x0008};

typedef struct vir0511h_data_s {
    struct i2c_client *client;
    struct mutex update_lock;
    bool valid;
    unsigned long last_updated;
    u16 temp_code[MAX_TEMP_NUM];
} vir0511h_data_t;

/* --------- I2C 操作：改用更穩定的 SMBus API --------- */
static int vir0511h_get_adc_code_by_index(struct i2c_client *client, int index, uint16_t *code)
{
    int ret;

    if (index >= MAX_TEMP_NUM)
        return -EINVAL;

    /* 1. 寫入 Config 暫存器 (0x01) 選擇通道 */
    /* 使用 _swapped 是因為 i2c-stub/x86 通常是 Little-endian，但裝置通常是 Big-endian */
    ret = i2c_smbus_write_word_swapped(client, VIR0511H_CONFIG_REG, def_config_data[index]);
    if (ret < 0)
        return ret;

    /* 需要延遲等待 ADC 轉換 */
    mdelay(1);

    /* 2. 讀取 Conversion 暫存器 (0x00) */
    ret = i2c_smbus_read_word_swapped(client, VIR0511H_CONVERSION_REG);
    if (ret < 0)
        return ret;

    *code = (uint16_t)ret;
    return 0;
}

/* --------- 更新 cache --------- */
static vir0511h_data_t *vir0511h_update_client(struct device *dev)
{
    vir0511h_data_t *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int i;

    mutex_lock(&data->update_lock);
    /* 1 秒內只更新一次 */
    if (!data->valid || time_after(jiffies, data->last_updated + HZ)) {
        for (i = 0; i < MAX_TEMP_NUM; i++) {
            vir0511h_get_adc_code_by_index(client, i, &data->temp_code[i]);
        }
        data->last_updated = jiffies;
        data->valid = true;
    }
    mutex_unlock(&data->update_lock);

    return data;
}

/* --------- hwmon_ops 實作 --------- */
static int vir0511h_read(struct device *dev, enum hwmon_sensor_types type,
                         u32 attr, int channel, long *val)
{
    vir0511h_data_t *data = vir0511h_update_client(dev);

    if (type == hwmon_temp && attr == hwmon_temp_input && channel < MAX_TEMP_NUM) {
        /* 讀取快取中的數值 */
        *val = data->temp_code[channel] & 0x7fff;
        return 0;
    }

    return -EOPNOTSUPP;
}

static umode_t vir0511h_is_visible(const void *data, enum hwmon_sensor_types type,
                                   u32 attr, int channel)
{
    if (type == hwmon_temp && attr == hwmon_temp_input && channel < MAX_TEMP_NUM)
        return 0444; /* 所有 4 個通道均唯讀可見 */
    return 0;
}

/* --------- 定義 hwmon channel：修正為 4 個通道 --------- */
static const struct hwmon_channel_info *vir0511h_info[] = {
    HWMON_CHANNEL_INFO(temp,
                       HWMON_T_INPUT,  /* temp1_input */
                       HWMON_T_INPUT,  /* temp2_input */
                       HWMON_T_INPUT,  /* temp3_input */
                       HWMON_T_INPUT), /* temp4_input */
    NULL
};

static const struct hwmon_ops vir0511h_hwmon_ops = {
    .is_visible = vir0511h_is_visible,
    .read = vir0511h_read,
};

static const struct hwmon_chip_info vir0511h_chip_info = {
    .ops = &vir0511h_hwmon_ops,
    .info = vir0511h_info,
};

/* --------- i2c probe / remove --------- */
static int vir0511h_probe(struct i2c_client *client)
{
    vir0511h_data_t *data;
    struct device *hwmon_dev;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    mutex_init(&data->update_lock);

    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev,
                                                     client->name,
                                                     data,
                                                     &vir0511h_chip_info,
                                                     NULL);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static void vir0511h_remove(struct i2c_client *client)
{
    /* devm 資源自動釋放 */
}

static const struct i2c_device_id vir0511h_id[] = {
    { "vir0511h", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, vir0511h_id);

#ifdef CONFIG_OF
static const struct of_device_id vir0511h_of_match[] = {
    { .compatible = "vir,vir0511h" },
    { }
};
MODULE_DEVICE_TABLE(of, of_device_id, vir0511h_of_match);
#endif

static struct i2c_driver vir0511h_driver = {
    .driver = {
        .name = "vir0511h",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(vir0511h_of_match),
    },
    .probe = vir0511h_probe,
    .remove = vir0511h_remove,
    .id_table = vir0511h_id,
};

module_i2c_driver(vir0511h_driver);

MODULE_DESCRIPTION("vir0511h HWMON Driver (SMBus Version)");
MODULE_LICENSE("GPL");