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

static char batteryinfo[48] = {0};

static struct proc_dir_entry *entry_binf = NULL;

static int fih_binf_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", batteryinfo);
	return 0;
}

static int fih_binf_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_binf_proc_read, inode->i_private);
}

static const struct proc_ops fih_binf_fops = {
	.proc_open    = fih_binf_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_binf_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_binf,batteryinfo", NULL);
	if (!p_chr) {
		pr_err("%s: batteryinfo not specified\n", __func__);
	} else {
		memset(batteryinfo, 0, sizeof(batteryinfo));
		strlcpy(batteryinfo, p_chr, sizeof(batteryinfo));
	}
	pr_info("%s: batteryinfo = (%s)\n", __func__, batteryinfo);

	return 0;
}

static int fih_binf_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_binf_property(pdev);

	entry_binf = proc_create("batteryinfo", 0444, NULL, &fih_binf_fops);
	if (!entry_binf) {
		pr_err("%s: failed to create proc batteryinfo\n", __func__);
	}

	return 0;
}

static int fih_binf_remove(struct platform_device *pdev)
{
	if (entry_binf) proc_remove(entry_binf);

	return 0;
}

static void fih_binf_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_binf_dt_match[] = {
	{ .compatible = "fih_binf" },
	{}
};

static struct platform_driver fih_binf_driver = {
	.probe    = fih_binf_probe,
	.remove   = fih_binf_remove,
	.shutdown = fih_binf_shutdown,
	.driver = {
		.name = "fih_binf",
		.owner = THIS_MODULE,
		.of_match_table = fih_binf_dt_match,
	},
};

static int __init fih_binf_init(void)
{
	return platform_driver_register(&fih_binf_driver);
}
module_init(fih_binf_init);

static void __exit fih_binf_exit(void)
{
	platform_driver_unregister(&fih_binf_driver);
}
module_exit(fih_binf_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
