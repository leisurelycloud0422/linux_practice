#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#define DRIVER_NAME "tmp102_sim"
#define TMP102_TEMP_REG 0x00

static int tmp102_sim_probe(struct i2c_client *client)
{
    s32 temp_val;

    pr_info("tmp102_sim: probe called for device at 0x%x\n", client->addr);

    temp_val = i2c_smbus_read_byte_data(client, TMP102_TEMP_REG);
    if (temp_val < 0) {
        pr_err("tmp102_sim: failed to read temp register\n");
        return temp_val;
    }
    pr_info("tmp102_sim: temperature register value = 0x%x\n", temp_val);

    return 0;
}

static void tmp102_sim_remove(struct i2c_client *client)
{
    pr_info("tmp102_sim: remove called\n");
}

static const struct i2c_device_id tmp102_sim_id[] = {
    { "tmp102_sim", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tmp102_sim_id);

static struct i2c_driver tmp102_sim_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe = tmp102_sim_probe,
    .remove = tmp102_sim_remove,
    .id_table = tmp102_sim_id,
};

module_i2c_driver(tmp102_sim_driver);

MODULE_AUTHOR("YourName");
MODULE_DESCRIPTION("TMP102 Simulated I2C Driver");
MODULE_LICENSE("GPL");
