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

static char productdate[16] = {0};  /* "yyyymmdd" */

static struct proc_dir_entry *entry_pdd = NULL;

static int fih_pdd_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", productdate);
	return 0;
}

static int fih_pdd_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_pdd_proc_read, inode->i_private);
}

static const struct proc_ops fih_pdd_fops = {
	.proc_open    = fih_pdd_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_pdd_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_pdd,productdate", NULL);
	if (!p_chr) {
		pr_err("%s: productdate not specified\n", __func__);
	} else {
		memset(productdate, 0, sizeof(productdate));
		strlcpy(productdate, p_chr, sizeof(productdate));
	}
	pr_info("%s: productdate = (%s)\n", __func__, productdate);

	return 0;
}

static int fih_pdd_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_pdd_property(pdev);

	entry_pdd = proc_create("ckdt", 0444, NULL, &fih_pdd_fops);
	if (!entry_pdd) {
		pr_err("%s: failed to create proc ckdt\n", __func__);
	}

	return 0;
}

static int fih_pdd_remove(struct platform_device *pdev)
{
	if (entry_pdd) proc_remove(entry_pdd);

	return 0;
}

static void fih_pdd_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_pdd_dt_match[] = {
	{ .compatible = "fih_pdd" },
	{}
};

static struct platform_driver fih_pdd_driver = {
	.probe = fih_pdd_probe,
	.remove = fih_pdd_remove,
	.shutdown = fih_pdd_shutdown,
	.driver = {
		.name = "fih_pdd",
		.owner = THIS_MODULE,
		.of_match_table = fih_pdd_dt_match,
	},
};

static int __init fih_pdd_init(void)
{
	return platform_driver_register(&fih_pdd_driver);
}
module_init(fih_pdd_init);

static void __exit fih_pdd_exit(void)
{
	platform_driver_unregister(&fih_pdd_driver);
}
module_exit(fih_pdd_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
