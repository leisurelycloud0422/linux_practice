#include "vir0511h.h"


static uint16_t def_config_data[MAX_TEMP_NUM] = {0x0001, 0x0002, 0x0004, 0x0008};
static int vir0511h_set_read_reg(struct i2c_client *client, uint8_t reg)
{
	unsigned char data;

	data = reg;
	printk("data=%d\n", data);
	return i2c_master_send(client, &data, sizeof(data));
}


static int vir0511h_i2c_read_data(struct i2c_client *client, int reg, void *dest, int bytes)
{
	int ret;

	if(NULL == dest)
	{
		printk(KERN_ERR"%s(%d):Input buffer is NULL\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	ret = vir0511h_set_read_reg(client, reg);
	if(ret != 0)
		return ret;


	return i2c_master_recv(client, dest, bytes);
}

static int vir0511h_set_config(struct i2c_client *client, uint16_t value)
{
    unsigned char buf[3];

    memset(buf, 0, sizeof(buf));

    buf[0] = VIR0511H_CONFIG_REG;
    buf[1] = value>>8;
    buf[2] = value&0x00ff;

    return i2c_master_send(client, buf, sizeof(buf));
}



static int vir0511h_get_adc_code_by_index(struct i2c_client *client, int index, uint16_t *code)
{
	int ret = 0;
	uint8_t code_buff[2];


	if(index >= MAX_TEMP_NUM)
		return -EINVAL;

	if((ret = vir0511h_set_config(client, def_config_data[index])) != 0)
		return ret;

	/*need sleep...*/
	memset(code_buff, 0, sizeof(code_buff));
	ret = vir0511h_i2c_read_data(client, VIR0511H_CONVERSION_REG, code_buff, sizeof(code_buff));
	if(ret != 0)
		return ret;

	*code = code_buff[0]<<8|code_buff[1];

	return ret;
}

static struct vir0511h_data_s *vir0511h_update_client(struct device *dev)
{
	struct vir0511h_data_s *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	mutex_lock(&data->update_lock);
	if (time_after(jiffies, data->last_updated) || !data->valid) {
		int i;

		for (i = 0; i < ARRAY_SIZE(data->temp_code); i++)
		{
			vir0511h_get_adc_code_by_index(client, i, &(data->temp_code[i]));
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);
	return data;
}


static ssize_t show_temp(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int ret = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct vir0511h_data_s *data = vir0511h_update_client(dev);
	ret = sprintf(buf, "%d\n", data->temp_code[attr->index]&0x7fff);

	printk("buf=%s temp_code=%d ret=%d\n", buf, data->temp_code[attr->index]&0x7fff, ret);
	return ret;
}
static umode_t vir0511h_attribute_visible(struct kobject *kobj,
		struct attribute *attr, int index)
{
	return attr->mode;
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_temp, NULL, 1);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, show_temp, NULL, 2);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO, show_temp, NULL, 3);

static struct attribute *vir0511h_attributes[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp4_input.dev_attr.attr,
	NULL
};
static const struct attribute_group vir0511h_group = {
	.attrs = vir0511h_attributes,
	.is_visible = vir0511h_attribute_visible
};
__ATTRIBUTE_GROUPS(vir0511h);


static int vir0511h_ctl_probe(struct i2c_client *client, const struct i2c_device_id *device_id)
{
	struct vir0511h_data_s *data;
	struct device *hwmon_dev;

	data = devm_kzalloc(&client->dev, sizeof(struct vir0511h_data_s),
		GFP_KERNEL);
	if(data == NULL)
		return -ENOMEM;

	data->client = client;
	mutex_init(&data->update_lock);
	hwmon_dev = devm_hwmon_device_register_with_groups(&client->dev,
							   client->name, data,
							   vir0511h_groups);


	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static int vir0511h_ctl_remove(struct i2c_client *client)
{

	printk("%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static const struct i2c_device_id vir0511h_ctl_id[] = 
{
	{ "vir0511h",0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, vir0511h_ctl_id);

#ifdef CONFIG_OF
static const struct of_device_id vir0511h_of_match[] = {
	{ .compatible = "vir, vir0511h" },
	{}
};
MODULE_DEVICE_TABLE(of, vir0511h_of_match);
#endif

static struct i2c_driver vir0511h_ctl_driver = {
	.driver = {
		.name	= "vir0511h",
		.owner	= THIS_MODULE,
		#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(vir0511h_of_match),
		#endif
	},
	.probe		= vir0511h_ctl_probe,
	.remove = vir0511h_ctl_remove,
	.id_table	= vir0511h_ctl_id,
};


static int __init vir0511h_ctl_init(void)
{
	return i2c_add_driver(&vir0511h_ctl_driver);
}

static void __exit vir0511h_ctl_exit(void)
{
	i2c_del_driver(&vir0511h_ctl_driver);
}
module_init(vir0511h_ctl_init);
module_exit(vir0511h_ctl_exit);
MODULE_DESCRIPTION("vir0511h control Drivers"); 
MODULE_LICENSE("GPL");
