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

static char draminfo[32] = {0};  //"UNKNOWN LPDDR4x 4096MB"
static char dramtest[8] = {0};   //"none", "pass" or "fail"

static struct proc_dir_entry *entry_dir = NULL;
static struct proc_dir_entry *entry_info = NULL;
static struct proc_dir_entry *entry_test = NULL;

static int fih_ddr_proc_read_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", draminfo);
	return 0;
}

static int fih_ddr_proc_open_info(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_ddr_proc_read_info, inode->i_private);
}

static const struct proc_ops fih_ddr_fops_info = {
	.proc_open    = fih_ddr_proc_open_info,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_ddr_proc_read_test(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", dramtest);
	return 0;
}

static int fih_ddr_proc_open_test(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_ddr_proc_read_test, inode->i_private);
}

static const struct proc_ops fih_ddr_fops_test = {
	.proc_open    = fih_ddr_proc_open_test,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_ddr_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_ddr,draminfo", NULL);
	if (!p_chr) {
		pr_err("%s: draminfo not specified\n", __func__);
	} else {
		memset(draminfo, 0, sizeof(draminfo));
		strlcpy(draminfo, p_chr, sizeof(draminfo));
	}
	pr_info("%s: draminfo = (%s)\n", __func__, draminfo);

	p_chr = of_get_property(pdev->dev.of_node, "fih_ddr,dramtest", NULL);
	if (!p_chr) {
		pr_err("%s: dramtest not specified\n", __func__);
	} else {
		memset(dramtest, 0, sizeof(dramtest));
		if (0 == strcmp(p_chr, "fail")) {
			snprintf(dramtest, sizeof(dramtest), "1");  /* fail */
		} else {
			snprintf(dramtest, sizeof(dramtest), "0");  /* pass */
		}
	}
	pr_info("%s: dramtest = (%s)\n", __func__, dramtest);

	return 0;
}

static int fih_ddr_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_ddr_property(pdev);

	entry_dir = proc_mkdir("AllHWList", NULL);
	if (!entry_dir) {
		pr_info("%s: failed to mkdir proc AllHWList\n", __func__);
		//return -EINVAL;
	}

	//entry_info = proc_create("draminfo", 0444, entry_dir, &fih_ddr_fops_info);
	entry_info = proc_create("AllHWList/draminfo", 0444, NULL, &fih_ddr_fops_info);
	if (!entry_info) {
		pr_err("%s: failed to create proc draminfo\n", __func__);
	}

	//entry_test = proc_create("dramtest_result", 0444, entry_dir, &fih_ddr_fops_test);
	entry_test = proc_create("AllHWList/dramtest_result", 0444, NULL, &fih_ddr_fops_test);
	if (!entry_test) {
		pr_err("%s: failed to create proc dramtest\n", __func__);
	}

	return 0;
}

static int fih_ddr_remove(struct platform_device *pdev)
{
	if (entry_test) proc_remove(entry_test);
	if (entry_info) proc_remove(entry_info);
	if (entry_dir)  proc_remove(entry_dir);

	return 0;
}

static void fih_ddr_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_ddr_dt_match[] = {
	{ .compatible = "fih_ddr" },
	{}
};

static struct platform_driver fih_ddr_driver = {
	.probe    = fih_ddr_probe,
	.remove   = fih_ddr_remove,
	.shutdown = fih_ddr_shutdown,
	.driver = {
		.name = "fih_ddr",
		.owner = THIS_MODULE,
		.of_match_table = fih_ddr_dt_match,
	},
};

static int __init fih_ddr_init(void)
{
	return platform_driver_register(&fih_ddr_driver);
}
module_init(fih_ddr_init);

static void __exit fih_ddr_exit(void)
{
	platform_driver_unregister(&fih_ddr_driver);
}
module_exit(fih_ddr_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
