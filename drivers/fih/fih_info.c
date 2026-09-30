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

static char devmodel[8] = {0};
static char baseband[8] = {0};
static char bandinfo[128] = {0};
static char hwmodel[8]  = {0};

static struct proc_dir_entry *entry_devmodel = NULL;
static struct proc_dir_entry *entry_baseband = NULL;
static struct proc_dir_entry *entry_bandinfo = NULL;
static struct proc_dir_entry *entry_hwmodel  = NULL;

static int fih_info_proc_show_devmodel(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", devmodel);
	return 0;
}

static int fih_info_proc_open_devmodel(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_info_proc_show_devmodel, inode->i_private);
}

static const struct proc_ops fih_info_fops_devmodel = {
	.proc_open    = fih_info_proc_open_devmodel,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_info_proc_show_baseband(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", baseband);
	return 0;
}

static int fih_info_proc_open_baseband(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_info_proc_show_baseband, inode->i_private);
}

static const struct proc_ops fih_info_fops_baseband = {
	.proc_open    = fih_info_proc_open_baseband,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_info_proc_show_bandinfo(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", bandinfo);
	return 0;
}

static int fih_info_proc_open_bandinfo(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_info_proc_show_bandinfo, inode->i_private);
}

static const struct proc_ops fih_info_fops_bandinfo = {
	.proc_open    = fih_info_proc_open_bandinfo,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_info_proc_show_hwmodel(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", hwmodel);
	return 0;
}

static int fih_info_proc_open_hwmodel(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_info_proc_show_hwmodel, inode->i_private);
}

static const struct proc_ops fih_info_fops_hwmodel = {
	.proc_open    = fih_info_proc_open_hwmodel,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_info_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_info,devmodel", NULL);
	if (!p_chr) {
		pr_err("%s: devmodel not specified\n", __func__);
	} else {
		memset(devmodel, 0, sizeof(devmodel));
		strlcpy(devmodel, p_chr, sizeof(devmodel));
	}
	pr_info("%s: devmodel = (%s)\n", __func__, devmodel);

	p_chr = of_get_property(pdev->dev.of_node, "fih_info,baseband", NULL);
	if (!p_chr) {
		pr_err("%s: baseband not specified\n", __func__);
	} else {
		memset(baseband, 0, sizeof(baseband));
		strlcpy(baseband, p_chr, sizeof(baseband));
	}
	pr_info("%s: baseband = (%s)\n", __func__, baseband);

	p_chr = of_get_property(pdev->dev.of_node, "fih_info,bandinfo", NULL);
	if (!p_chr) {
		pr_err("%s: bandinfo not specified\n", __func__);
	} else {
		memset(bandinfo, 0, sizeof(bandinfo));
		strlcpy(bandinfo, p_chr, sizeof(bandinfo));
	}
	pr_info("%s: bandinfo = (%s)\n", __func__, bandinfo);

	p_chr = of_get_property(pdev->dev.of_node, "fih_info,hwmodel", NULL);
	if (!p_chr) {
		pr_err("%s: hwmodel not specified\n", __func__);
	} else {
		memset(hwmodel, 0, sizeof(hwmodel));
		strlcpy(hwmodel, p_chr, sizeof(hwmodel));
	}
	pr_info("%s: hwmodel = (%s)\n", __func__, hwmodel);

	return 0;
}

static int fih_info_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_info_property(pdev);

	entry_devmodel = proc_create("devmodel", 0444, NULL, &fih_info_fops_devmodel);
	if (!entry_devmodel) {
		pr_err("%s: failed to create proc devmodel\n", __func__);
	}

	entry_baseband = proc_create("baseband", 0444, NULL, &fih_info_fops_baseband);
	if (!entry_baseband) {
		pr_err("%s: failed to create proc baseband\n", __func__);
	}

	entry_bandinfo = proc_create("bandinfo", 0444, NULL, &fih_info_fops_bandinfo);
	if (!entry_bandinfo) {
		pr_err("%s: failed to create proc bandinfo\n", __func__);
	}

	entry_hwmodel = proc_create("hwmodel",  0444, NULL, &fih_info_fops_hwmodel);
	if (!entry_hwmodel) {
		pr_err("%s: failed to create proc hwmodel\n", __func__);
	}

	return 0;
}

static int fih_info_remove(struct platform_device *pdev)
{
	if (entry_hwmodel)  proc_remove(entry_hwmodel);
	if (entry_bandinfo) proc_remove(entry_bandinfo);
	if (entry_baseband) proc_remove(entry_baseband);
	if (entry_devmodel) proc_remove(entry_devmodel);

	return 0;
}

static void fih_info_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_info_dt_match[] = {
	{ .compatible = "fih_info" },
	{}
};

static struct platform_driver fih_info_driver = {
	.probe = fih_info_probe,
	.remove = fih_info_remove,
	.shutdown = fih_info_shutdown,
	.driver = {
		.name = "fih_info",
		.owner = THIS_MODULE,
		.of_match_table = fih_info_dt_match,
	},
};

static int __init fih_info_init(void)
{
	return platform_driver_register(&fih_info_driver);
}
module_init(fih_info_init);

static void __exit fih_info_exit(void)
{
	platform_driver_unregister(&fih_info_driver);
}
module_exit(fih_info_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
