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

static char ufsinfo[32] = {0};  /* "UNKNOWN UFS 64GB" */

static struct proc_dir_entry *entry_dir = NULL;
static struct proc_dir_entry *entry_ufs = NULL;

static int fih_ufs_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", ufsinfo);
	return 0;
}

static int fih_ufs_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_ufs_proc_read, NULL);
}

static const struct proc_ops fih_ufs_fops = {
	.proc_open    = fih_ufs_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_ufs_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_ufs,ufsinfo", NULL);
	if (!p_chr) {
		pr_err("%s: ufsinfo not specified\n", __func__);
	} else {
		memset(ufsinfo, 0, sizeof(ufsinfo));
		strlcpy(ufsinfo, p_chr, sizeof(ufsinfo));
	}
	pr_info("%s: ufsinfo = (%s)\n", __func__, ufsinfo);

	return 0;
}

static int fih_ufs_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_ufs_property(pdev);

	entry_dir = proc_mkdir("AllHWList", NULL);
	if (!entry_dir) {
		pr_info("%s: failed to mkdir proc AllHWList\n", __func__);
		//return -EINVAL;
	}

	//entry_ufs = proc_create("ufsinfo", 0444, entry_dir, &fih_ufs_fops);
	entry_ufs = proc_create("AllHWList/ufsinfo", 0444, NULL, &fih_ufs_fops);
	if (!entry_ufs) {
		pr_err("%s: failed to create proc ufsinfo\n", __func__);
	}

	return 0;
}

static int fih_ufs_remove(struct platform_device *pdev)
{
	if (entry_ufs) proc_remove(entry_ufs);
	if (entry_dir) proc_remove(entry_dir);

	return 0;
}

static void fih_ufs_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_ufs_dt_match[] = {
	{ .compatible = "fih_ufs" },
	{}
};

static struct platform_driver fih_ufs_driver = {
	.probe = fih_ufs_probe,
	.remove = fih_ufs_remove,
	.shutdown = fih_ufs_shutdown,
	.driver = {
		.name = "fih_ufs",
		.owner = THIS_MODULE,
		.of_match_table = fih_ufs_dt_match,
	},
};

static int __init fih_ufs_init(void)
{
	return platform_driver_register(&fih_ufs_driver);
}
module_init(fih_ufs_init);

static void __exit fih_ufs_exit(void)
{
	platform_driver_unregister(&fih_ufs_driver);
}
module_exit(fih_ufs_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
