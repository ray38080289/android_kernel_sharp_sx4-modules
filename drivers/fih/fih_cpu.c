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

static char cpuinfo[32] = {0};

static struct proc_dir_entry *entry_dir = NULL;
static struct proc_dir_entry *entry_cpu = NULL;

static int fih_cpu_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", cpuinfo);
	return 0;
}

static int fih_cpu_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_cpu_proc_read, inode->i_private);
}

static const struct proc_ops fih_cpu_fops = {
	.proc_open    = fih_cpu_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_cpu_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_cpu,cpuinfo", NULL);
	if (!p_chr) {
		pr_err("%s: cpuinfo not specified\n", __func__);
	} else {
		memset(cpuinfo, 0, sizeof(cpuinfo));
		strlcpy(cpuinfo, p_chr, sizeof(cpuinfo));
	}
	pr_info("%s: cpuinfo = (%s)\n", __func__, cpuinfo);

	return 0;
}

static int fih_cpu_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_cpu_property(pdev);

	entry_dir = proc_mkdir("AllHWList", NULL);
	if (!entry_dir) {
		pr_info("%s: failed to mkdir proc AllHWList\n", __func__);
		//return -EINVAL;
	}

	//entry_cpu = proc_create("cpuinfo", 0444, entry_dir, &fih_cpu_fops);
	entry_cpu = proc_create("AllHWList/cpuinfo", 0444, NULL, &fih_cpu_fops);
	if (!entry_cpu) {
		pr_err("%s: failed to create proc cpuinfo\n", __func__);
	}

	return 0;
}

static int fih_cpu_remove(struct platform_device *pdev)
{
	if (entry_cpu) proc_remove(entry_cpu);
	if (entry_dir) proc_remove(entry_dir);

	return 0;
}

static void fih_cpu_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_cpu_dt_match[] = {
	{ .compatible = "fih_cpu" },
	{}
};

static struct platform_driver fih_cpu_driver = {
	.probe    = fih_cpu_probe,
	.remove   = fih_cpu_remove,
	.shutdown = fih_cpu_shutdown,
	.driver = {
		.name = "fih_cpu",
		.owner = THIS_MODULE,
		.of_match_table = fih_cpu_dt_match,
	},
};

static int __init fih_cpu_init(void)
{
	return platform_driver_register(&fih_cpu_driver);
}
module_init(fih_cpu_init);

static void __exit fih_cpu_exit(void)
{
	platform_driver_unregister(&fih_cpu_driver);
}
module_exit(fih_cpu_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
