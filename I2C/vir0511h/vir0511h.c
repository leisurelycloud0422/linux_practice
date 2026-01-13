#include "vir0511h.h"
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

static uint16_t def_config_data[MAX_TEMP_NUM] = {0x0001, 0x0002, 0x0004, 0x0008};

typedef struct vir0511h_data_s {
    struct i2c_client *client;
    struct mutex update_lock;
    char valid;
    unsigned long last_updated;
    uint16_t temp_code[MAX_TEMP_NUM];
} vir0511h_data_t;

/* --------- I2C 低階操作 --------- */
static int vir0511h_set_read_reg(struct i2c_client *client, uint8_t reg)
{
    return i2c_master_send(client, &reg, sizeof(reg));
}

static int vir0511h_i2c_read_data(struct i2c_client *client, int reg, void *dest, int bytes)
{
    int ret;

    if (!dest)
        return -EINVAL;

    ret = vir0511h_set_read_reg(client, reg);
    if (ret)
        return ret;

    return i2c_master_recv(client, dest, bytes);
}

static int vir0511h_set_config(struct i2c_client *client, uint16_t value)
{
    uint8_t buf[3];

    buf[0] = VIR0511H_CONFIG_REG;
    buf[1] = value >> 8;
    buf[2] = value & 0xFF;

    return i2c_master_send(client, buf, sizeof(buf));
}

static int vir0511h_get_adc_code_by_index(struct i2c_client *client, int index, uint16_t *code)
{
    int ret;
    uint8_t code_buff[2];

    if (index >= MAX_TEMP_NUM)
        return -EINVAL;

    ret = vir0511h_set_config(client, def_config_data[index]);
    if (ret)
        return ret;

    /* 需要延遲等待 ADC */
    mdelay(1);

    ret = vir0511h_i2c_read_data(client, VIR0511H_CONVERSION_REG, code_buff, sizeof(code_buff));
    if (ret)
        return ret;

    *code = (code_buff[0] << 8) | code_buff[1];
    return 0;
}

/* --------- 更新 cache --------- */
static vir0511h_data_t *vir0511h_update_client(struct device *dev)
{
    vir0511h_data_t *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int i;

    mutex_lock(&data->update_lock);
    if (!data->valid || time_after(jiffies, data->last_updated + HZ)) {
        for (i = 0; i < MAX_TEMP_NUM; i++)
            vir0511h_get_adc_code_by_index(client, i, &data->temp_code[i]);
        data->last_updated = jiffies;
        data->valid = 1;
    }
    mutex_unlock(&data->update_lock);

    return data;
}

/* --------- hwmon_ops 實作 --------- */
static int vir0511h_read(struct device *dev,
                         enum hwmon_sensor_types type,
                         u32 attr, int channel, long *val)
{
    vir0511h_data_t *data = vir0511h_update_client(dev);

    if (type == hwmon_temp && attr == hwmon_temp_input && channel < MAX_TEMP_NUM) {
        *val = data->temp_code[channel] & 0x7fff;
        return 0;
    }

    return -EOPNOTSUPP;
}

static umode_t vir0511h_is_visible(const void *data,
                                   enum hwmon_sensor_types type,
                                   u32 attr, int channel)
{
    if (type == hwmon_temp && attr == hwmon_temp_input && channel < MAX_TEMP_NUM)
        return 0444; /* 可讀 */
    return 0;
}

/* --------- 定義 hwmon channel --------- */
static const struct hwmon_channel_info *vir0511h_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
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
static int vir0511h_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
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

static int vir0511h_remove(struct i2c_client *client)
{
    return 0;
}

/* --------- i2c 驅動註冊 --------- */
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

/* --------- module init / exit --------- */
module_i2c_driver(vir0511h_driver);

MODULE_DESCRIPTION("vir0511h HWMON Driver (new API)");
MODULE_LICENSE("GPL");
