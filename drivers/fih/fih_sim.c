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

static char simslot[8] = {0};

static struct proc_dir_entry *entry_sim = NULL;

static int fih_sim_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", simslot);
	return 0;
}

static int fih_sim_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_sim_proc_read, inode->i_private);
}

static const struct proc_ops fih_sim_fops = {
	.proc_open    = fih_sim_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_sim_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_sim,simslot", NULL);
	if (!p_chr) {
		pr_err("%s: simslot not specified\n", __func__);
	} else {
		memset(simslot, 0, sizeof(simslot));
		strlcpy(simslot, p_chr, sizeof(simslot));
	}
	pr_info("%s: simslot = (%s)\n", __func__, simslot);

	return 0;
}

static int fih_sim_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_sim_property(pdev);

	entry_sim = proc_create("SIMSlot", 0444, NULL, &fih_sim_fops);
	if (!entry_sim) {
		pr_err("%s: failed to create proc SIMSlot\n", __func__);
	}

	return 0;
}

static int fih_sim_remove(struct platform_device *pdev)
{
	if (entry_sim) proc_remove(entry_sim);

	return 0;
}

static void fih_sim_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_sim_dt_match[] = {
	{ .compatible = "fih_sim" },
	{}
};

static struct platform_driver fih_sim_driver = {
	.probe = fih_sim_probe,
	.remove = fih_sim_remove,
	.shutdown = fih_sim_shutdown,
	.driver = {
		.name = "fih_sim",
		.owner = THIS_MODULE,
		.of_match_table = fih_sim_dt_match,
	},
};

static int __init fih_sim_init(void)
{
	return platform_driver_register(&fih_sim_driver);
}
module_init(fih_sim_init);

static void __exit fih_sim_exit(void)
{
	platform_driver_unregister(&fih_sim_driver);
}
module_exit(fih_sim_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
