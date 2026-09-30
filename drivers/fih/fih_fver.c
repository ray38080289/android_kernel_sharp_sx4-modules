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

static char systeminfo[128] = {0};

static struct proc_dir_entry *entry_fver = NULL;

static int fih_fver_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", systeminfo);
	return 0;
}

static int fih_fver_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_fver_proc_read, inode->i_private);
}

static const struct proc_ops fih_fver_fops = {
	.proc_open    = fih_fver_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_fver_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_fver,systeminfo", NULL);
	if (!p_chr) {
		pr_info("%s: systeminfo not specified\n", __func__);
	} else {
		memset(systeminfo, 0, sizeof(systeminfo));
		strlcpy(systeminfo, p_chr, sizeof(systeminfo));
	}
	pr_info("%s: systeminfo = (%s)\n", __func__, systeminfo);

	return 0;
}

static int fih_fver_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_fver_property(pdev);

	entry_fver = proc_create("fver", 0444, NULL, &fih_fver_fops);
	if (!entry_fver) {
		pr_err("%s: failed to create proc fver\n", __func__);
	}

	return 0;
}

static int fih_fver_remove(struct platform_device *pdev)
{
	if (entry_fver) proc_remove(entry_fver);

	return 0;
}

static void fih_fver_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_fver_dt_match[] = {
	{ .compatible = "fih_fver" },
	{}
};

static struct platform_driver fih_fver_driver = {
	.probe = fih_fver_probe,
	.remove = fih_fver_remove,
	.shutdown = fih_fver_shutdown,
	.driver = {
		.name = "fih_fver",
		.owner = THIS_MODULE,
		.of_match_table = fih_fver_dt_match,
	},
};

static int __init fih_fver_init(void)
{
	return platform_driver_register(&fih_fver_driver);
}
module_init(fih_fver_init);

static void __exit fih_fver_exit(void)
{
	platform_driver_unregister(&fih_fver_driver);
}
module_exit(fih_fver_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
