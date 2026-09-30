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

#include "mt-plat/mtk_thermal_monitor.h"

#if 1
#define mtk_cooler_tethering_dprintk(fmt, args...)	\
	pr_notice("thermal/cooler/tethering " fmt, ##args)
#else
#define mtk_cooler_tethering_dprintk(fmt, args...)
#endif

#define MAX_NUM_INSTANCE_MTK_COOLER_TETHERING  8
static DEFINE_MUTEX(tethering_off_count_mutex);
static int tethering_off_count = 0;

static struct thermal_cooling_device *cl_tethering_dev[MAX_NUM_INSTANCE_MTK_COOLER_TETHERING] = { 0 };
static unsigned long cl_tethering_state[MAX_NUM_INSTANCE_MTK_COOLER_TETHERING] = { 0 };

static int mtk_cl_tethering_get_max_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	/* mtk_cooler_tethering_dprintk("mtk_cl_tethering_get_max_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_tethering_get_cur_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = *((unsigned long *)cdev->devdata);
	/* mtk_cooler_tethering_dprintk("mtk_cl_tethering_get_cur_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_tethering_set_cur_state (struct thermal_cooling_device *cdev, unsigned long state)
{
	unsigned long original_state = *((unsigned long *)cdev->devdata);

	/* mtk_cooler_tethering_dprintk("mtk_cl_tethering_warn_set_cur_state() %s %d to %d\n", cdev->type, original_state, state); */
	*((unsigned long *)cdev->devdata) = state;

	if(state != original_state) {
		mutex_lock(&tethering_off_count_mutex);
		if (state == 1) {
			if (tethering_off_count == 0) {
				char event[20] = "TETHERING=0";
				char *envp[2] = { event, NULL };

				mtk_cooler_tethering_dprintk("%s %s notify tethering restrict by uevent [%s]\n", __func__, cdev->type, event);
				kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
			}
			tethering_off_count++;
			mtk_cooler_tethering_dprintk("%s %s tethering restrict, count %d\n", __func__, cdev->type, tethering_off_count);
		} else {
			tethering_off_count--;
			mtk_cooler_tethering_dprintk("%s %s tethering restrict release, count %d\n", __func__, cdev->type, tethering_off_count);
			if (tethering_off_count == 0) {
				char event[20] = "TETHERING=1";
				char *envp[2] = { event, NULL };

				mtk_cooler_tethering_dprintk("%s %s notify tethering restrict release by uevent [%s]\n", __func__, cdev->type, event);
				kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
			}
		}
		mutex_unlock(&tethering_off_count_mutex);
	}

	return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_tethering_ops = {
	.get_max_state = mtk_cl_tethering_get_max_state,
	.get_cur_state = mtk_cl_tethering_get_cur_state,
	.set_cur_state = mtk_cl_tethering_set_cur_state,
};

static int mtk_cooler_tethering_register_ltf(void)
{
	int i;

	mtk_cooler_tethering_dprintk("register ltf\n");

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_TETHERING; i-- > 0;) {
		char temp[20] = { 0 };

		sprintf(temp, "tethering%02d", i);
		cl_tethering_dev[i] = mtk_thermal_cooling_device_register(
				temp, (void *)&cl_tethering_state[i],
				&mtk_cl_tethering_ops);
	}

	return 0;
}

static void mtk_cooler_tethering_unregister_ltf(void)
{
	int i;

	mtk_cooler_tethering_dprintk("unregister ltf\n");

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_TETHERING; i-- > 0;) {
		if (cl_tethering_dev[i]) {
			mtk_thermal_cooling_device_unregister(
					cl_tethering_dev[i]);
			cl_tethering_dev[i] = NULL;
			cl_tethering_state[i] = 0;
		}
	}
}


int mtk_cooler_tethering_init(void)
{
	int err = 0;
	int i;

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_TETHERING; i-- > 0;) {
		cl_tethering_dev[i] = NULL;
		cl_tethering_state[i] = 0;
	}

	mtk_cooler_tethering_dprintk("init\n");

	err = mtk_cooler_tethering_register_ltf();
	if (err)
		goto err_unreg;

	return 0;

err_unreg:
	mtk_cooler_tethering_unregister_ltf();
	return err;
}

void mtk_cooler_tethering_exit(void)
{
	mtk_cooler_tethering_dprintk("exit\n");

	mtk_cooler_tethering_unregister_ltf();
}
//module_init(mtk_cooler_tethering_init);
//module_exit(mtk_cooler_tethering_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
