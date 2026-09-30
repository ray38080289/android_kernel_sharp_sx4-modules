// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/leds-mtk.h>

#include "mt-plat/mtk_thermal_monitor.h"

#define mtk_cooler_backlight_dprintk(fmt, args...)	\
	pr_notice("thermal/cooler/backlight " fmt, ##args)

#define BACKLIGHT_COOLER_NR 3

static struct thermal_cooling_device
*cl_backlight_dev[BACKLIGHT_COOLER_NR] = { 0 };

static unsigned int g_cl_backlight_state[BACKLIGHT_COOLER_NR] = { 0 };

/* static unsigned int g_cl_backlight_last_state[BACKLIGHT_COOLER_NR] = {0}; */
static unsigned int g_cl_id[BACKLIGHT_COOLER_NR];
static unsigned int g_backlight_level;
static unsigned int g_backlight_last_level;
static unsigned int g_backlight_restrict_notified = 0;

//FIH-Thermal-Brigntness-Node+[
static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);
#define MAX_LEN 256
static unsigned int g_backlight_restrict_value = 255;
static ssize_t _mtk_cl_bl_max_brightness_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
        char desc[MAX_LEN] = { 0 };
        int tmp_value = -1;

        len = (len < (MAX_LEN - 1)) ? len : (MAX_LEN - 1);
        /* write data to the buffer */
        if (copy_from_user(desc, buf, len))
                return -EFAULT;

        if (kstrtoint(desc, 10, &tmp_value) == 0) {
                if (tmp_value >= 0 && tmp_value <= 255) {
                        g_backlight_restrict_value = tmp_value;
                } else {
                        mtk_cooler_backlight_dprintk("[%s] oo range %s = %d\n", __func__, desc, g_backlight_restrict_value);
	                return -EFAULT;
		}
        } else {
                mtk_cooler_backlight_dprintk("[%s] bad arg %s = %d\n", __func__, desc, g_backlight_restrict_value);
                return -EFAULT;
        }
        mtk_cooler_backlight_dprintk("[%s] %s = %d\n", __func__, desc, g_backlight_restrict_value);

        setMaxBrightness(-1, (g_backlight_restrict_value*100)/255, 1);

        return len;
}

static int _mtk_cl_bl_max_brightness_read(struct seq_file *m, void *v)
{
        seq_printf(m, "%d\n", g_backlight_restrict_value);
        mtk_cooler_backlight_dprintk("[%s] %d\n", __func__, g_backlight_restrict_value);

        return 0;
}

static int _mtk_cl_bl_max_brightness_open(struct inode *inode, struct file *file)
{
        return single_open(file, _mtk_cl_bl_max_brightness_read, pde_data(inode));
}

static const struct proc_ops _cl_bl_max_brightness_fops = {
        //.proc_owner = THIS_MODULE,
        .proc_open = _mtk_cl_bl_max_brightness_open,
        .proc_read = seq_read,
        .proc_lseek = seq_lseek,
        .proc_write = _mtk_cl_bl_max_brightness_write,
        .proc_release = single_release,
};
//FIH-Thermal-Brigntness-Node+]

static void mtk_cl_backlight_set_max_brightness_limit(void)
{
	if (g_backlight_last_level != g_backlight_level) {
		mtk_cooler_backlight_dprintk("set brightness level = %d\n",
				g_backlight_level);

		switch (g_backlight_level) {
		case 0:
			/* 100% */
			g_backlight_restrict_value = 255; //FIH-Thermal-Brigntness-Node+
			setMaxBrightness(-1, 100, 0);
			break;
		case 1:
			/* 70% */
			g_backlight_restrict_value = 178; //FIH-Thermal-Brigntness-Node+
			setMaxBrightness(-1, 70, 0);
			break;
		case 2:
			/* 40% */
			g_backlight_restrict_value = 102; //FIH-Thermal-Brigntness-Node+
			setMaxBrightness(-1, 40, 1);
			break;
		case 3:
			/* 10% */
			g_backlight_restrict_value = 25; //FIH-Thermal-Brigntness-Node+
			setMaxBrightness(-1, 10, 1);
			break;
		default:
			g_backlight_restrict_value = 255; //FIH-Thermal-Brigntness-Node+
			setMaxBrightness(-1, 100, 0);
			break;
		}
	}
}

	static int mtk_cl_backlight_get_max_state
(struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	/* mtk_cooler_backlight_dprintk
	 * ("mtk_cl_backlight_get_max_state() %d\n", *state);
	 */
	return 0;
}

	static int mtk_cl_backlight_get_cur_state
(struct thermal_cooling_device *cdev, unsigned long *state)
{
	int nCoolerId;

	/* Get Cooler ID */
	nCoolerId = *((int *)cdev->devdata);

	*state = g_cl_backlight_state[nCoolerId];
	/* mtk_cooler_backlight_dprintk
	 * ("mtk_cl_backlight_get_cur_state() %d CoolerID:%d\n",
	 * state, nCoolerId);
	 */
	return 0;
}

	static int mtk_cl_backlight_set_cur_state
(struct thermal_cooling_device *cdev, unsigned long state)
{
	int i;
	int nCoolerId;		/* /< Backlight Cooler ID */

	/* Get Cooler ID */
	nCoolerId = *((int *)cdev->devdata);

	/* mtk_cooler_backlight_dprintk
	 * ("mtk_cl_backlight_set_cur_state() %d CoolerID:%d\n",
	 * state, nCoolerId);
	 */

	g_cl_backlight_state[nCoolerId] = state;

	g_backlight_level = 0;
	for (i = 0; i < BACKLIGHT_COOLER_NR; i++)
		g_backlight_level += g_cl_backlight_state[i];

	/* Mark for test */
	if(g_backlight_last_level != g_backlight_level)
	{
		/* send uevent to notify current call must be dropped
		 */
		char event[20] = {0};
		char *envp[] = { event, NULL };
		/*
		if(g_backlight_last_level < g_backlight_level) { // increase mitigation level (SHTHERMAL_LCDBRIGHTNESS_EMERGENCY)
			sprintf(event, "BACKLIGHT=0");
		} else { // decrease mitigation level (SHTHERMAL_LCDBRIGHTNESS_NORMAL)
			sprintf(event, "BACKLIGHT=1");
		}
		*/

		if(g_backlight_level == BACKLIGHT_COOLER_NR) { // Restrict (SHTHERMAL_LCDBRIGHTNESS_EMERGENCY)
			sprintf(event, "BACKLIGHT=0");
			kobject_uevent_env(&(cl_backlight_dev[nCoolerId]->device.kobj),	KOBJ_CHANGE, envp);
			g_backlight_restrict_notified = 1;
//#ifdef CONFIG_FIH_SX4
			printk("FIHBATTLOG::125\n"); // 125 SHTHERMAL_LCD_RESTRICT
//#endif
		} else if(g_backlight_level == 0 && g_backlight_restrict_notified == 1) { // Release (SHTHERMAL_LCDBRIGHTNESS_NORMAL)
			sprintf(event, "BACKLIGHT=1");
			kobject_uevent_env(&(cl_backlight_dev[nCoolerId]->device.kobj),	KOBJ_CHANGE, envp);
			g_backlight_restrict_notified = 0;
//#ifdef CONFIG_FIH_SX4
			printk("FIHBATTLOG::126\n"); // 126 SHTHERMAL_LCD_RELEASE
//#endif
		}

		mtk_cl_backlight_set_max_brightness_limit();

		g_backlight_last_level = g_backlight_level;

		mtk_cooler_backlight_dprintk("mtk_cl_backlight_set_cur_state() event:%s g_backlight_level:%d\n", event, g_backlight_level);
		/* mtk_cooler_backlight_dprintk
		 * ("mtk_cl_backlight_set_cur_state()
		 * event:%s g_backlight_level:%d\n",
		 * event, g_backlight_level);
		 */

	}

	return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_backlight_ops = {
	.get_max_state = mtk_cl_backlight_get_max_state,
	.get_cur_state = mtk_cl_backlight_get_cur_state,
	.set_cur_state = mtk_cl_backlight_set_cur_state,
};

static int mtk_cooler_backlight_register_ltf(void)
{
	int i;

	mtk_cooler_backlight_dprintk("register ltf\n");

	for (i = 0; i < BACKLIGHT_COOLER_NR; i++) {
		char temp[20] = { 0 };

		sprintf(temp, "mtk-cl-backlight%02d", i + 1);
		/* /< Cooler Name: mtk-cl-backlight01 */

		g_cl_id[i] = i;
		cl_backlight_dev[i] = mtk_thermal_cooling_device_register
			(temp, (void *)&g_cl_id[i],
			 &mtk_cl_backlight_ops);
	}

	return 0;
}

static void mtk_cooler_backlight_unregister_ltf(void)
{
	int i;

	mtk_cooler_backlight_dprintk("unregister ltf\n");

	for (i = 0; i < BACKLIGHT_COOLER_NR; i++) {
		if (cl_backlight_dev[i]) {
			mtk_thermal_cooling_device_unregister
				(cl_backlight_dev[i]);
			cl_backlight_dev[i] = NULL;
		}
	}
}


int mtk_cooler_backlight_init(void)
{
	int err = 0;

	mtk_cooler_backlight_dprintk("init\n");

	err = mtk_cooler_backlight_register_ltf();
	if (err)
		goto err_unreg;

//FIH-Thermal-Brigntness-Node+[
	{
		struct proc_dir_entry *entry = NULL;
		struct proc_dir_entry *dir_entry = mtk_thermal_get_proc_drv_therm_dir_entry();

		if (!dir_entry) {
			mtk_cooler_backlight_dprintk( "%s mkdir /proc/driver/thermal failed\n", __func__);
			return 0;
		}

		entry = proc_create("clbl_max_brightness", 0664, dir_entry, &_cl_bl_max_brightness_fops);
		if (!entry)
			mtk_cooler_backlight_dprintk("%s clbl_max_brightness creation failed\n", __func__);
		else
			proc_set_user(entry, uid, gid);
	}
//FIH-Thermal-Brigntness-Node+]

	return 0;

err_unreg:
	mtk_cooler_backlight_unregister_ltf();
	return err;
}

void mtk_cooler_backlight_exit(void)
{
	mtk_cooler_backlight_dprintk("exit\n");

	mtk_cooler_backlight_unregister_ltf();
}
// module_init(mtk_cooler_backlight_init);
// module_exit(mtk_cooler_backlight_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
