#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/kernel.h> 
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define MAX_TEMP_NUM 4

#define VIR0511H_CONVERSION_REG     0x00
#define VIR0511H_CONFIG_REG         0x01
typedef struct vir0511h_data_s
{
	struct i2c_client *client;
	struct mutex update_lock;
	char valid;			
	unsigned long last_updated;
	uint16_t temp_code[MAX_TEMP_NUM];
}vir0511h_data_t;
