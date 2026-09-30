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

static char enabled[32]  = {0};
static char serialno[32] = {0};
static char unlocked[32] = {0};

static struct proc_dir_entry *entry_secboot  = NULL;
static struct proc_dir_entry *entry_enabled  = NULL;
static struct proc_dir_entry *entry_serialno = NULL;
static struct proc_dir_entry *entry_unlocked = NULL;

static int fih_seb_proc_read_enabled(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", enabled);
	return 0;
}

static int fih_seb_proc_open_enabled(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_seb_proc_read_enabled, inode->i_private);
}

static const struct proc_ops fih_seb_fops_enabled = {
	.proc_open    = fih_seb_proc_open_enabled,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_seb_proc_read_serialno(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", serialno);
	return 0;
}

static int fih_seb_proc_open_serialno(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_seb_proc_read_serialno, inode->i_private);
}

static const struct proc_ops fih_seb_fops_serialno = {
	.proc_open    = fih_seb_proc_open_serialno,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_seb_proc_read_unlocked(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", unlocked);
	return 0;
}

static int fih_seb_proc_open_unlocked(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_seb_proc_read_unlocked, inode->i_private);
}

static const struct proc_ops fih_seb_fops_unlocked = {
	.proc_open    = fih_seb_proc_open_unlocked,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_seb_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_seb,enabled", NULL);
	if (!p_chr) {
		pr_err("%s: enabled not specified\n", __func__);
	} else {
		memset(enabled, 0, sizeof(enabled));
		strlcpy(enabled, p_chr, sizeof(enabled));
	}
	pr_info("%s: enabled = (%s)\n", __func__, enabled);

	p_chr = of_get_property(pdev->dev.of_node, "fih_seb,serialno", NULL);
	if (!p_chr) {
		pr_err("%s: serialno not specified\n", __func__);
	} else {
		memset(serialno, 0, sizeof(serialno));
		strlcpy(serialno, p_chr, sizeof(serialno));
	}
	pr_info("%s: serialno = (%s)\n", __func__, serialno);

	p_chr = of_get_property(pdev->dev.of_node, "fih_seb,unlocked", NULL);
	if (!p_chr) {
		pr_err("%s: unlocked not specified\n", __func__);
	} else {
		memset(unlocked, 0, sizeof(unlocked));
		strlcpy(unlocked, p_chr, sizeof(unlocked));
	}
	pr_info("%s: unlocked = (%s)\n", __func__, unlocked);

	return 0;
}

static int fih_seb_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_seb_property(pdev);

	entry_secboot = proc_mkdir("secboot", NULL);
	if (!entry_secboot) {
		pr_err("%s: failed to mkdir proc secboot\n", __func__);
		return -EINVAL;
	}

	entry_enabled = proc_create("enabled", 0444, entry_secboot, &fih_seb_fops_enabled);
	if (!entry_enabled) {
		pr_err("%s: failed to create proc enabled\n", __func__);
	}

	entry_serialno = proc_create("serialno", 0444, entry_secboot, &fih_seb_fops_serialno);
	if (!entry_serialno) {
		pr_err("%s: failed to create proc serialno\n", __func__);
	}

	entry_unlocked = proc_create("unlocked", 0444, entry_secboot, &fih_seb_fops_unlocked);
	if (!entry_unlocked) {
		pr_err("%s: failed to create proc unlocked\n", __func__);
	}

	return 0;
}

static int fih_seb_remove(struct platform_device *pdev)
{
	if (entry_unlocked) proc_remove(entry_unlocked);
	if (entry_serialno) proc_remove(entry_serialno);
	if (entry_enabled)  proc_remove(entry_enabled);
	if (entry_secboot)  proc_remove(entry_secboot);

	return 0;
}

static void fih_seb_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_seb_dt_match[] = {
	{ .compatible = "fih_seb" },
	{}
};

static struct platform_driver fih_seb_driver = {
	.probe = fih_seb_probe,
	.remove = fih_seb_remove,
	.shutdown = fih_seb_shutdown,
	.driver = {
		.name = "fih_seb",
		.owner = THIS_MODULE,
		.of_match_table = fih_seb_dt_match,
	},
};

static int __init fih_seb_init(void)
{
	return platform_driver_register(&fih_seb_driver);
}
module_init(fih_seb_init);

static void __exit fih_seb_exit(void)
{
	platform_driver_unregister(&fih_seb_driver);
}
module_exit(fih_seb_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
