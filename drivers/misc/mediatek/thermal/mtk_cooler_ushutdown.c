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
#include <linux/uidgid.h>

#include "mt-plat/mtk_thermal_monitor.h"

#if 1
#define mtk_cooler_ushutdown_dprintk(fmt, args...)	\
	pr_notice("thermal/cooler/ushutdown " fmt, ##args)
#else
#define mtk_cooler_ushutdown_dprintk(fmt, args...)
#endif

#define MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN 8
static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);

enum tsd_state {
	TSD_INIT,
	TSD_SHUTDOWN_COUNTING,
	TSD_CLEAR_COUNTING,
};

struct timer_shutdown_state {
	struct thermal_cooling_device *cdev;
	unsigned long state;
	char tz_type[THERMAL_NAME_LENGTH];
	int threshold_a; // shutdown threshold
	int threshold_b; // clear threshold
	time64_t timer_a; // shutdown timer
	time64_t timer_b; // clear timer
	time64_t alarm_time; // alarm time (time before shutdown)
	/* internal */
	int tz_idx;
	enum tsd_state t_state;
	int ui_warn_shown;
	time64_t count_time_a; // count for timer a
	time64_t start_time_a;
	time64_t start_time_b;
};
static int global_ui_warn_shown = 0;

static struct timer_shutdown_state cl_ushutdown_timer_state[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };

static struct thermal_cooling_device *cl_ushutdown_dev[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };
static unsigned long cl_ushutdown_state[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };

static struct thermal_cooling_device *cl_ushutdown_warn_dev[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };
static unsigned long cl_ushutdown_warn_state[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };
static time64_t cl_ushutdown_warn_ts_start[MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN] = { 0 };

static void notify_thermal_shutdown(const char *src, struct thermal_cooling_device *cdev) {
	static bool is_notified = false;
	if (is_notified == false) {
		char event[20] = "SHUTDOWN=1";
		char *envp[2] = { event, NULL };
		/* send uevent to notify current call must be dropped */
		kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
		is_notified = true;
		mtk_cooler_ushutdown_dprintk("%s trigger shutdown", src);
//#ifdef CONFIG_FIH_SX4
		printk("FIHBATTLOG::124\n"); // 124 SHTHERMAL_POWER_OFF
//#endif
	} else {
		mtk_cooler_ushutdown_dprintk("%s trigger shutdown (silence)", src);
	}
}

static ssize_t _cltsd_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	/* int ret = 0; */
	char tmp[128] = { 0 };
	char tz_type[THERMAL_NAME_LENGTH] = { 0 };
	int cl_idx, tz_idx, threshold_a, threshold_b, timer_a, timer_b, alarm_time;

	len = (len < (128 - 1)) ? len : (128 - 1);
	/* write data to the buffer */
	if (copy_from_user(tmp, buf, len)) {
		mtk_cooler_ushutdown_dprintk("%s copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	if (data == NULL) {
		mtk_cooler_ushutdown_dprintk("%s null data\n", __func__);
		return -EINVAL;
	}

	if (sscanf(tmp, "%d %s %d %d %d %d %d", &cl_idx, tz_type, &threshold_a, &threshold_b, &timer_a, &timer_b, &alarm_time) >= 1) {
		if (cl_idx < 0 || cl_idx >= MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN) {
			mtk_cooler_ushutdown_dprintk("%s wrong timer shutdown index %d\n", __func__, cl_idx);
			return -EINVAL;
		}

		tz_idx = mtk_thermal_get_tz_idx(tz_type);
		if (tz_idx == -1) {
			mtk_cooler_ushutdown_dprintk("%s wrong tz %s (tz_idx %d)\n", __func__, tz_type, tz_idx);
			cl_ushutdown_timer_state[cl_idx].state = 0;
			memset(cl_ushutdown_timer_state[cl_idx].tz_type, 0, THERMAL_NAME_LENGTH);
			cl_ushutdown_timer_state[cl_idx].tz_idx = -1;
			cl_ushutdown_timer_state[cl_idx].threshold_a = -274000;
			cl_ushutdown_timer_state[cl_idx].threshold_b = -274000;
			cl_ushutdown_timer_state[cl_idx].timer_a = 0;
			cl_ushutdown_timer_state[cl_idx].timer_b = 0;
			cl_ushutdown_timer_state[cl_idx].alarm_time = 0;
			cl_ushutdown_timer_state[cl_idx].t_state = TSD_INIT;
			cl_ushutdown_timer_state[cl_idx].count_time_a = 0;
			cl_ushutdown_timer_state[cl_idx].start_time_a = 0;
			cl_ushutdown_timer_state[cl_idx].start_time_b = 0;
			if ((cl_ushutdown_timer_state[cl_idx].ui_warn_shown == 1) && (global_ui_warn_shown > 0))
				global_ui_warn_shown--;
			cl_ushutdown_timer_state[cl_idx].ui_warn_shown = 0;
			return -EINVAL;
		}

		if (threshold_b >= threshold_a) {
			mtk_cooler_ushutdown_dprintk("%s wrong threshold setting: threshold_b %d must be less than threshold_a %d\n", __func__, threshold_b, threshold_a);
			return -EINVAL;
		}

		if ((timer_a <= 0) || (timer_b <= 0) || (alarm_time <= 0)) {
			mtk_cooler_ushutdown_dprintk("%s wrong timer setting: timer_a %d and timer_b %d and alarm_time %d must be larger than 0\n", __func__, timer_a, timer_b, alarm_time);
			return -EINVAL;
		}

		if (alarm_time > timer_a) {
			mtk_cooler_ushutdown_dprintk("%s wrong timer setting: timer_a %d shall be larger than or equal to alarm_time %d\n", __func__, timer_a, alarm_time);
			return -EINVAL;
		}

		memset(cl_ushutdown_timer_state[cl_idx].tz_type, 0, THERMAL_NAME_LENGTH);
		strncpy(cl_ushutdown_timer_state[cl_idx].tz_type, tz_type, THERMAL_NAME_LENGTH);
		cl_ushutdown_timer_state[cl_idx].tz_idx = tz_idx;
		cl_ushutdown_timer_state[cl_idx].threshold_a = threshold_a;
		cl_ushutdown_timer_state[cl_idx].threshold_b = threshold_b;
		cl_ushutdown_timer_state[cl_idx].timer_a = timer_a;
		cl_ushutdown_timer_state[cl_idx].timer_b = timer_b;
		cl_ushutdown_timer_state[cl_idx].alarm_time = alarm_time;
		cl_ushutdown_timer_state[cl_idx].t_state = TSD_INIT;
		cl_ushutdown_timer_state[cl_idx].count_time_a = 0;
		cl_ushutdown_timer_state[cl_idx].start_time_a = 0;
		cl_ushutdown_timer_state[cl_idx].start_time_b = 0;
		if ((cl_ushutdown_timer_state[cl_idx].ui_warn_shown == 1) && (global_ui_warn_shown > 0))
			global_ui_warn_shown--;
		cl_ushutdown_timer_state[cl_idx].ui_warn_shown = 0;
		return len;
	}

	mtk_cooler_ushutdown_dprintk("%s bad argument\n", __func__);
	return -EINVAL;
}

static int _cltsd_read(struct seq_file *m, void *v)
{
	/* mtk_cooler_ushutdown_dprintk("%s\n", __func__); */

	seq_printf(m, "global_ui_warn_shown %d (%d)\n", (global_ui_warn_shown > 0)?1:0, global_ui_warn_shown);
	{
		int i;

		for (i = 0; i < MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i++) {
			seq_printf(m, "[%02d] state %lu (tz_idx %d)(T %d): [ %s %d %d %lld %lld %lld ] %d %lld %lld %d\n", i,
				cl_ushutdown_timer_state[i].state,
				cl_ushutdown_timer_state[i].tz_idx,
				mtk_thermal_get_temp(cl_ushutdown_timer_state[i].tz_idx),
				(cl_ushutdown_timer_state[i].tz_idx == -1)?"none":cl_ushutdown_timer_state[i].tz_type,
				cl_ushutdown_timer_state[i].threshold_a,
				cl_ushutdown_timer_state[i].threshold_b,
				cl_ushutdown_timer_state[i].timer_a,
				cl_ushutdown_timer_state[i].timer_b,
				cl_ushutdown_timer_state[i].alarm_time,
				cl_ushutdown_timer_state[i].t_state,
				cl_ushutdown_timer_state[i].start_time_a?(ktime_get_seconds() - cl_ushutdown_timer_state[i].start_time_a + cl_ushutdown_timer_state[i].count_time_a):cl_ushutdown_timer_state[i].count_time_a,
				cl_ushutdown_timer_state[i].start_time_b?(ktime_get_seconds() - cl_ushutdown_timer_state[i].start_time_b):0,
				cl_ushutdown_timer_state[i].ui_warn_shown);
		}
	}

	return 0;
}

static int _cltsd_open(struct inode *inode, struct file *file)
{
	return single_open(file, _cltsd_read, pde_data(inode));
}

static const struct proc_ops _cltsd_fops = {
	.proc_open = _cltsd_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_write = _cltsd_write,
	.proc_release = single_release,
};

static int mtk_cl_ushutdown_timer_get_max_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_timer_get_max_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_ushutdown_timer_get_cur_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	int i = 0, cl_idx = -1;
	for (i = 0; i < MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i++) {
		if (cdev == cl_ushutdown_timer_state[i].cdev) {
			cl_idx = i;
			break;
		}
	}
	if (cl_idx == -1) {
		mtk_cooler_ushutdown_dprintk("%s found no timer_shutdown_state for %s\n", __func__, cdev->type);
		return -1;
	}
	*state = (cl_ushutdown_timer_state[cl_idx].t_state != TSD_INIT)?1:0;
	//*state = *((unsigned long *)cdev->devdata);
	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_timer_get_cur_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_ushutdown_timer_set_cur_state (struct thermal_cooling_device *cdev, unsigned long state)
{
	int i, cl_idx = -1, curr_temp = -274000;
	time64_t curr_time = 0;
	unsigned long prev_state = *((unsigned long *)cdev->devdata);

	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_timer_set_cur_state() %s %d (prev %d)\n", cdev->type, state, prev_state); */
	*((unsigned long *)cdev->devdata) = state;

	for (i = 0; i < MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i++) {
		if (cdev == cl_ushutdown_timer_state[i].cdev) {
			cl_idx = i;
			break;
		}
	}
	if (cl_idx == -1) {
		mtk_cooler_ushutdown_dprintk("%s found no timer_shutdown_state for %s\n", __func__, cdev->type);
		return -1;
	}

	if (cl_ushutdown_timer_state[i].tz_idx == -1) {
		mtk_cooler_ushutdown_dprintk("%s no valid TZ for %s\n", __func__, cdev->type);
		return -1;
	}

	curr_temp = mtk_thermal_get_temp(cl_ushutdown_timer_state[i].tz_idx);
	curr_time = ktime_get_seconds();
	switch (cl_ushutdown_timer_state[i].t_state) {
		case TSD_INIT:
			if (curr_temp >= cl_ushutdown_timer_state[i].threshold_a) {
				cl_ushutdown_timer_state[i].count_time_a = 0;
				cl_ushutdown_timer_state[i].start_time_a = curr_time;
				cl_ushutdown_timer_state[i].start_time_b = 0;
				cl_ushutdown_timer_state[i].t_state = TSD_SHUTDOWN_COUNTING;
			}
			break;
		case TSD_SHUTDOWN_COUNTING:
			if (curr_temp < cl_ushutdown_timer_state[i].threshold_a) {
				cl_ushutdown_timer_state[i].count_time_a += (curr_time - cl_ushutdown_timer_state[i].start_time_a);
				cl_ushutdown_timer_state[i].start_time_a = 0;
				cl_ushutdown_timer_state[i].start_time_b = curr_time;
				cl_ushutdown_timer_state[i].t_state = TSD_CLEAR_COUNTING;
			} else {
				if ((curr_time - cl_ushutdown_timer_state[i].start_time_a + cl_ushutdown_timer_state[i].count_time_a) > (cl_ushutdown_timer_state[i].timer_a - cl_ushutdown_timer_state[i].alarm_time) &&
				    (cl_ushutdown_timer_state[i].ui_warn_shown == 0)) {

					if(global_ui_warn_shown == 0) {
						char event[20] = "SHUTDOWN=0";
						char *envp[2] = { event, NULL };

						mtk_cooler_ushutdown_dprintk("%s %s notify shutdown-in-3-min by uevent [%s]\n", __func__, cdev->type, event);
						kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
//#ifdef CONFIG_FIH_SX4
						printk("FIHBATTLOG::123\n"); // 123 SHTHERMAL_POWER_OFF_DIALOG
//#endif
					}
					cl_ushutdown_timer_state[i].ui_warn_shown = 1;
					global_ui_warn_shown++;
				} else if ((curr_time - cl_ushutdown_timer_state[i].start_time_a + cl_ushutdown_timer_state[i].count_time_a) > cl_ushutdown_timer_state[i].timer_a) {
					notify_thermal_shutdown(__func__, cdev);
#if 0
					static bool is_notified = false;
					if (is_notified == false) {
						char event[20] = "SHUTDOWN=1";
						char *envp[2] = { event, NULL };

						mtk_cooler_ushutdown_dprintk("%s %s notify shutdown by uevent [%s]\n", __func__, cdev->type, event);
//#ifdef CONFIG_FIH_SX4
						printk("FIHBATTLOG::124\n"); // 124 SHTHERMAL_POWER_OFF
//#endif
						kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
						is_notified = true;
					}
#endif
				}
			}
			break;
		case TSD_CLEAR_COUNTING:
			if (curr_temp >= cl_ushutdown_timer_state[i].threshold_a) {
				cl_ushutdown_timer_state[i].start_time_a = curr_time;
				cl_ushutdown_timer_state[i].start_time_b = 0;
				cl_ushutdown_timer_state[i].t_state = TSD_SHUTDOWN_COUNTING;
			} else if ((curr_temp < cl_ushutdown_timer_state[i].threshold_b) ||
				   ((curr_time - cl_ushutdown_timer_state[i].start_time_b) > cl_ushutdown_timer_state[i].timer_b)) {
				cl_ushutdown_timer_state[i].count_time_a = 0;
				cl_ushutdown_timer_state[i].start_time_a = 0;
				cl_ushutdown_timer_state[i].start_time_b = 0;
				if (cl_ushutdown_timer_state[i].ui_warn_shown == 1) {
					cl_ushutdown_timer_state[i].ui_warn_shown = 0;
					if (global_ui_warn_shown > 0)
						global_ui_warn_shown--;
				}
				cl_ushutdown_timer_state[i].t_state = TSD_INIT;
			}
			break;
		default:
			mtk_cooler_ushutdown_dprintk("%s unknown current tsd_state %d, state %lu, prev_state %lu\n", __func__, cl_ushutdown_timer_state[i].t_state, state, prev_state);
	}
	return 0;
}

static int mtk_cl_ushutdown_get_max_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_get_max_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_ushutdown_get_cur_state (struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = *((unsigned long *)cdev->devdata);
	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_get_cur_state() %s %d\n", cdev->type, *state); */
	return 0;
}

static int mtk_cl_ushutdown_set_cur_state (struct thermal_cooling_device *cdev, unsigned long state)
{
	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_set_cur_state() %s %d\n", cdev->type, state); */
	*((unsigned long *)cdev->devdata) = state;

	if (state == 1) {
		notify_thermal_shutdown(__func__, cdev);
#if 0
		static bool is_notified = false;
		if (is_notified == false) {
			char event[20] = "SHUTDOWN=1";
			char *envp[2] = { event, NULL };

			mtk_cooler_ushutdown_dprintk("%s %s notify shutdown by uevent [%s]\n", __func__, cdev->type, event);
//#ifdef CONFIG_FIH_SX4
			printk("FIHBATTLOG::124\n"); // 124 SHTHERMAL_POWER_OFF
//#endif
			kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
			is_notified = true;
		}
#endif
	}

	return 0;
}

static int mtk_cl_ushutdown_warn_set_cur_state (struct thermal_cooling_device *cdev, unsigned long state)
{
	unsigned long original_state = *((unsigned long *)cdev->devdata);

	/* mtk_cooler_ushutdown_dprintk("mtk_cl_ushutdown_warn_set_cur_state() %s %d to %d\n", cdev->type, original_state, state); */
	*((unsigned long *)cdev->devdata) = state;

	if (state == 1) {
		int i = 0;

		for (i = 0; i < MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i++)
			if (cdev == cl_ushutdown_warn_dev[i])
				break;
		if (i == MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN) {
			mtk_cooler_ushutdown_dprintk("%s ERROR: cdev %s is not in cl_ushutdown_warn_dev[]\n", __func__, cdev->type);
			return 1;
		}

		if(state != original_state) {
			char event[20] = "SHUTDOWN=0";
			char *envp[2] = { event, NULL };

			mtk_cooler_ushutdown_dprintk("%s %s notify shutdown-in-3-min by uevent [%s]\n", __func__, cdev->type, event);
//#ifdef CONFIG_FIH_SX4
			printk("FIHBATTLOG::123\n"); // 123 SHTHERMAL_POWER_OFF_DIALOG
//#endif
			kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
			cl_ushutdown_warn_ts_start[i] =  ktime_get_seconds();
		} else {
			if ((ktime_get_seconds() - cl_ushutdown_warn_ts_start[i]) >= 180) { // High Temp Alarm keep for 3 min
				notify_thermal_shutdown(__func__, cdev);
#if 0
				static bool is_notified = false;
				if (is_notified == false) {
					char event[20] = "SHUTDOWN=1";
					char *envp[2] = { event, NULL };

					mtk_cooler_ushutdown_dprintk("%s %s notify shutdown by uevent [%s]\n", __func__, cdev->type, event);
//#ifdef CONFIG_FIH_SX4
					printk("FIHBATTLOG::124\n"); // 124 SHTHERMAL_POWER_OFF
//#endif
					kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
					is_notified = true;
				}
#endif
			}
		}

	}

	return 0;
}

static struct thermal_cooling_device_ops mtk_cl_ushutdown_timer_ops = {
	.get_max_state = mtk_cl_ushutdown_timer_get_max_state,
	.get_cur_state = mtk_cl_ushutdown_timer_get_cur_state,
	.set_cur_state = mtk_cl_ushutdown_timer_set_cur_state,
};
static struct thermal_cooling_device_ops mtk_cl_ushutdown_ops = {
	.get_max_state = mtk_cl_ushutdown_get_max_state,
	.get_cur_state = mtk_cl_ushutdown_get_cur_state,
	.set_cur_state = mtk_cl_ushutdown_set_cur_state,
};
static struct thermal_cooling_device_ops mtk_cl_ushutdown_warn_ops = {
	.get_max_state = mtk_cl_ushutdown_get_max_state,
	.get_cur_state = mtk_cl_ushutdown_get_cur_state,
	.set_cur_state = mtk_cl_ushutdown_warn_set_cur_state,
};

static int mtk_cooler_ushutdown_register_ltf(void)
{
	int i;

	mtk_cooler_ushutdown_dprintk("register ltf\n");

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i-- > 0;) {
		char temp[20] = { 0 };

		sprintf(temp, "ushutdown%02d", i);
		cl_ushutdown_dev[i] = mtk_thermal_cooling_device_register(
				temp, (void *)&cl_ushutdown_state[i],
				&mtk_cl_ushutdown_ops);

		sprintf(temp, "ushutdown-warn%02d", i);
		cl_ushutdown_warn_dev[i] = mtk_thermal_cooling_device_register(
				temp, (void *)&cl_ushutdown_warn_state[i],
				&mtk_cl_ushutdown_warn_ops);

		sprintf(temp, "ushutdown-timer%02d", i);
		cl_ushutdown_timer_state[i].cdev = mtk_thermal_cooling_device_register(
				temp, (void *)&cl_ushutdown_timer_state[i].state,
				&mtk_cl_ushutdown_timer_ops);
	}

	return 0;
}

static void mtk_cooler_ushutdown_unregister_ltf(void)
{
	int i;

	mtk_cooler_ushutdown_dprintk("unregister ltf\n");

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i-- > 0;) {
		if (cl_ushutdown_dev[i]) {
			mtk_thermal_cooling_device_unregister(
					cl_ushutdown_dev[i]);
			cl_ushutdown_dev[i] = NULL;
			cl_ushutdown_state[i] = 0;
		}
		if (cl_ushutdown_warn_dev[i]) {
			mtk_thermal_cooling_device_unregister(
					cl_ushutdown_warn_dev[i]);
			cl_ushutdown_warn_dev[i] = NULL;
			cl_ushutdown_warn_state[i] = 0;
			cl_ushutdown_warn_ts_start[i] = 0;
		}
		if (cl_ushutdown_timer_state[i].cdev) {
			mtk_thermal_cooling_device_unregister(
					cl_ushutdown_timer_state[i].cdev);
			cl_ushutdown_timer_state[i].cdev = NULL;
			cl_ushutdown_timer_state[i].state = 0;
			memset(cl_ushutdown_timer_state[i].tz_type, 0, THERMAL_NAME_LENGTH);
			cl_ushutdown_timer_state[i].tz_idx = -1;
			cl_ushutdown_timer_state[i].threshold_a = -274000;
			cl_ushutdown_timer_state[i].threshold_b = -274000;
			cl_ushutdown_timer_state[i].timer_a = 0;
			cl_ushutdown_timer_state[i].timer_b = 0;
			cl_ushutdown_timer_state[i].alarm_time = 0;
			cl_ushutdown_timer_state[i].t_state = TSD_INIT;
			cl_ushutdown_timer_state[i].count_time_a = 0;
			cl_ushutdown_timer_state[i].start_time_a = 0;
			cl_ushutdown_timer_state[i].start_time_b = 0;
			cl_ushutdown_timer_state[i].ui_warn_shown = 0;
		}
		global_ui_warn_shown = 0;
	}
}


int mtk_cooler_ushutdown_init(void)
{
	int err = 0;
	int i;

	for (i = MAX_NUM_INSTANCE_MTK_COOLER_USHUTDOWN; i-- > 0;) {
		cl_ushutdown_dev[i] = NULL;
		cl_ushutdown_state[i] = 0;

		cl_ushutdown_warn_dev[i] = NULL;
		cl_ushutdown_warn_state[i] = 0;
		cl_ushutdown_warn_ts_start[i] = 0;

		cl_ushutdown_timer_state[i].cdev = NULL;
		cl_ushutdown_timer_state[i].state = 0;
		memset(cl_ushutdown_timer_state[i].tz_type, 0, THERMAL_NAME_LENGTH);
		cl_ushutdown_timer_state[i].tz_idx = -1;
		cl_ushutdown_timer_state[i].threshold_a = -274000;
		cl_ushutdown_timer_state[i].threshold_b = -274000;
		cl_ushutdown_timer_state[i].timer_a = 0;
		cl_ushutdown_timer_state[i].timer_b = 0;
		cl_ushutdown_timer_state[i].alarm_time = 0;
		cl_ushutdown_timer_state[i].t_state = TSD_INIT;
		cl_ushutdown_timer_state[i].count_time_a = 0;
		cl_ushutdown_timer_state[i].start_time_a = 0;
		cl_ushutdown_timer_state[i].start_time_b = 0;
		cl_ushutdown_timer_state[i].ui_warn_shown = 0;
	}
	global_ui_warn_shown = 0;

	{
		struct proc_dir_entry *entry = NULL;
		struct proc_dir_entry *dir_entry = mtk_thermal_get_proc_drv_therm_dir_entry();

		if (!dir_entry) {
			mtk_cooler_ushutdown_dprintk("%s mkdir /proc/driver/thermal failed\n", __func__);
			return 0;
		}

		entry =	proc_create("cltsd_param", 0664, dir_entry, &_cltsd_fops);
		if (!entry)
			mtk_cooler_ushutdown_dprintk("%s cltsd_param creation failed\n", __func__);
		else
			proc_set_user(entry, uid, gid);
	}

	mtk_cooler_ushutdown_dprintk("init\n");

	err = mtk_cooler_ushutdown_register_ltf();
	if (err)
		goto err_unreg;

	return 0;

err_unreg:
	mtk_cooler_ushutdown_unregister_ltf();
	return err;
}

void mtk_cooler_ushutdown_exit(void)
{
	mtk_cooler_ushutdown_dprintk("exit\n");

	mtk_cooler_ushutdown_unregister_ltf();
}
//module_init(mtk_cooler_ushutdown_init);
//module_exit(mtk_cooler_ushutdown_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
