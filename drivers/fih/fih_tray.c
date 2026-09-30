#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regmap.h>
#include <linux/reboot.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>

#if IS_ENABLED(CONFIG_DEVICE_MODULES_MMC_MTK)
/* MTK/LINUX/android/kernel/kernel_device_modules-6.6/drivers/mmc/host/mtk-mmc.c */
extern int msdc_get_tray(void);
#endif

static struct proc_dir_entry *entry_dir = NULL;
static struct proc_dir_entry *entry_mmc = NULL;

static int fih_tray_read(struct seq_file *m, void *v)
{
#if IS_ENABLED(CONFIG_DEVICE_MODULES_MMC_MTK)
	seq_printf(m, "%d\n", msdc_get_tray());  /* 0=REMOVE , 1=INSERT */
#else
	seq_printf(m, "-1\n");  /* ERROR */
#endif
	return 0;
}

static int fih_tray_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_tray_read, inode->i_private);
}

static const struct proc_ops fih_tray_fops = {
	.proc_open    = fih_tray_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init fih_tray_init(void)
{
	entry_dir = proc_mkdir("AllHWList", NULL);
	if (!entry_dir) {
		pr_info("%s: failed to mkdir proc AllHWList\n", __func__);
		//return -EINVAL;
	}

	//entry_mmc = proc_create("tray_status", 0444, entry_dir, &fih_tray_fops);
	entry_mmc = proc_create("AllHWList/tray_status", 0444, NULL, &fih_tray_fops);
	if (!entry_mmc) {
		pr_err("%s: failed to create proc tray_status\n", __func__);
	}

	return 0;
}
module_init(fih_tray_init);

static void __exit fih_tray_exit(void)
{
	if (entry_mmc) proc_remove(entry_mmc);
	if (entry_dir) proc_remove(entry_dir);
}
module_exit(fih_tray_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
