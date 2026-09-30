// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>


/* ----------------------------------------------------------------- */
/* AW99703 Backlight Driver Implementations */
/* ----------------------------------------------------------------- */
#define I2C_BACKLIGHT_ID_NAME  "aw99703"

#if defined(CONFIG_MTK_LEGACY)
#err
#define AW99703_I2C_BUSNUM    7
#define AW99703_ADDR          0x36
static struct i2c_board_info aw99703_board_info __initdata = {
	I2C_BOARD_INFO(I2C_BACKLIGHT_ID_NAME, AW99703_ADDR)
};
#else
static const struct of_device_id lcm_of_match_backlight[] = {
		{.compatible = "mediatek,i2c_lcd_backlight"},
		{},
};
MODULE_DEVICE_TABLE(of, lcm_of_match_backlight);
#endif

static struct i2c_client *aw99703_i2c_client;

static int aw99703_probe(struct i2c_client *client/*, const struct i2c_device_id *id*/);
static void aw99703_remove(struct i2c_client *client);

struct aw99703_dev {
	struct i2c_client *client;
};

static const struct i2c_device_id aw99703_id[] = {
	{I2C_BACKLIGHT_ID_NAME, 0},
	{}
};

static struct i2c_driver aw99703_iic_driver = {
	.id_table = aw99703_id,
	.probe = aw99703_probe,
	.remove = aw99703_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = I2C_BACKLIGHT_ID_NAME,
#if !defined(CONFIG_MTK_LEGACY)
			.of_match_table = lcm_of_match_backlight,
#endif
		   },
};

static int aw99703_probe(struct i2c_client *client/*, const struct i2c_device_id *id*/)
{
	pr_err("[Kernel/LCM] %s enter, info==>name=%s addr=0x%x\n", __func__, client->name, client->addr);
	aw99703_i2c_client = client;
	return 0;
}

static void aw99703_remove(struct i2c_client *client)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	aw99703_i2c_client = NULL;
	i2c_unregister_device(client);
	
}

static int aw99703_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = aw99703_i2c_client;
	char write_data[2] = { 0 };

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_err("[Kernel/LCM] AW99703 write data fail !!\n");
	return ret;
}

void sx3_aw99703_init_reg(void)
{
	aw99703_write_bytes(0x02, 0x01); // MODE
	aw99703_write_bytes(0x06, 0x07); // Brightness Register LSBs (3bits)
	aw99703_write_bytes(0x07, 0xFF); // Brightness Register MSBs (8bits)
	aw99703_write_bytes(0x08, 0x93); // PWM Control Register
	aw99703_write_bytes(0x03, 0xB7); // LEDCUR Enable
}
EXPORT_SYMBOL(sx3_aw99703_init_reg);

void aw99703_init_reg(void)
{
	aw99703_write_bytes(0x02, 0x01); // MODE
	aw99703_write_bytes(0x06, 0x07); // Brightness Register LSBs (3bits)
	aw99703_write_bytes(0x07, 0xFF); // Brightness Register MSBs (8bits)
	aw99703_write_bytes(0x08, 0x93); // PWM Control Register
	aw99703_write_bytes(0x03, 0xB3); // LEDCUR Enable
}
EXPORT_SYMBOL(aw99703_init_reg);

static int __init aw99703_iic_init(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(AW99703_I2C_BUSNUM, &aw99703_board_info, 1);
#endif
	i2c_add_driver(&aw99703_iic_driver);
	pr_err("[Kernel/LCM] %s success\n", __func__);
	return 0;
}

static void __exit aw99703_iic_exit(void)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);
	i2c_del_driver(&aw99703_iic_driver);
}

module_init(aw99703_iic_init);
module_exit(aw99703_iic_exit);

MODULE_AUTHOR("FIH");
MODULE_DESCRIPTION("AW99703 I2C Driver");
MODULE_LICENSE("GPL");