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

static char pon[16]  = {0};  /* power on cause */
static char poff[16] = {0};  /* power off cause */
static char rere[16] = {0};  /* reboot reason */

static char bm[32] = {0};  /* boot_mode */
static char br[32] = {0};  /* boot_reason */

static struct proc_dir_entry *entry_pon  = NULL;
static struct proc_dir_entry *entry_poff = NULL;
static struct proc_dir_entry *entry_rere = NULL;

static ssize_t fih_apr_proc_write_pon(struct file *filp,
	const char *ubuf, size_t cnt, loff_t *data)
{
	unsigned char tmp[16];
	unsigned int len, i;

	len = (cnt > sizeof(tmp))? sizeof(tmp):cnt;

	if (copy_from_user(tmp, ubuf, len)) {
		pr_err("%s: copy_from_user fail\n", __func__);
		return 0;
	}

	/* remove unprintable characters */
	for (i = 0; i < sizeof(tmp); i++) {
		if ((tmp[i] < 0x20)||(0x7E < tmp[i])) {
			tmp[i] = 0x00;
		}
	}

	memset(pon, 0, sizeof(pon));
	strlcpy(pon, tmp, sizeof(pon));

	return len;
}

static int fih_apr_proc_read_pon(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", pon);
	return 0;
}

static int fih_apr_proc_open_pon(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_apr_proc_read_pon, inode->i_private);
}

static const struct proc_ops fih_apr_fops_pon = {
	.proc_open    = fih_apr_proc_open_pon,
	.proc_write   = fih_apr_proc_write_pon,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_apr_proc_read_poff(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", poff);
	return 0;
}

static int fih_apr_proc_open_poff(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_apr_proc_read_poff, inode->i_private);
}

static const struct proc_ops fih_apr_fops_poff = {
	.proc_open    = fih_apr_proc_open_poff,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_apr_proc_read_rere(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", rere);
	return 0;
}

static int fih_apr_proc_open_rere(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_apr_proc_read_rere, inode->i_private);
}

static const struct proc_ops fih_apr_fops_rere = {
	.proc_open    = fih_apr_proc_open_rere,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_apr_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_apr,pon", NULL);
	if (!p_chr) {
		pr_err("%s: pon not specified\n", __func__);
	} else {
		memset(pon, 0, sizeof(pon));
		strlcpy(pon, p_chr, sizeof(pon));
	}
	pr_info("%s: poweroncause = (%s)\n", __func__, pon);

	p_chr = of_get_property(pdev->dev.of_node, "fih_apr,poff", NULL);
	if (!p_chr) {
		pr_err("%s: poff not specified\n", __func__);
	} else {
		memset(poff, 0, sizeof(poff));
		strlcpy(poff, p_chr, sizeof(poff));
	}
	pr_info("%s: poweroffcause = (%s)\n", __func__, poff);

	p_chr = of_get_property(pdev->dev.of_node, "fih_apr,rere", NULL);
	if (!p_chr) {
		pr_err("%s: rere not specified\n", __func__);
	} else {
		memset(rere, 0, sizeof(rere));
		strlcpy(rere, p_chr, sizeof(rere));
	}
	pr_info("%s: rebootreason = (%s)\n", __func__, rere);

	p_chr = of_get_property(pdev->dev.of_node, "fih_apr,bm", NULL);
	if (!p_chr) {
		pr_err("%s: bm not specified\n", __func__);
	} else {
		memset(bm, 0, sizeof(bm));
		strlcpy(bm, p_chr, sizeof(bm));
	}
	pr_info("%s: boot_mode = (%s)\n", __func__, bm);

	p_chr = of_get_property(pdev->dev.of_node, "fih_apr,br", NULL);
	if (!p_chr) {
		pr_err("%s: br not specified\n", __func__);
	} else {
		memset(br, 0, sizeof(br));
		strlcpy(br, p_chr, sizeof(br));
	}
	pr_info("%s: boot_reason = (%s)\n", __func__, br);

	return 0;
}

static int fih_apr_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_apr_property(pdev);

	entry_pon = proc_create("poweroncause", 0664, NULL, &fih_apr_fops_pon);
	if (!entry_pon) {
		pr_err("%s: failed to create proc poweroncause\n", __func__);
	}

	entry_poff = proc_create("poweroffcause", 0444, NULL, &fih_apr_fops_poff);
	if (!entry_poff) {
		pr_err("%s: failed to create proc poweroffcause\n", __func__);
	}

	entry_rere = proc_create("rebootreason", 0444, NULL, &fih_apr_fops_rere);
	if (!entry_rere) {
		pr_err("%s: failed to create proc rebootreason\n", __func__);
	}

	return 0;
}

static int fih_apr_remove(struct platform_device *pdev)
{
	if (entry_rere) proc_remove(entry_rere);
	if (entry_poff) proc_remove(entry_poff);
	if (entry_pon)  proc_remove(entry_pon);

	return 0;
}

static void fih_apr_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_apr_dt_match[] = {
	{ .compatible = "fih_apr" },
	{}
};

static struct platform_driver fih_apr_driver = {
	.probe    = fih_apr_probe,
	.remove   = fih_apr_remove,
	.shutdown = fih_apr_shutdown,
	.driver = {
		.name = "fih_apr",
		.owner = THIS_MODULE,
		.of_match_table = fih_apr_dt_match,
	},
};

static int __init fih_apr_init(void)
{
	return platform_driver_register(&fih_apr_driver);
}
module_init(fih_apr_init);

static void __exit fih_apr_exit(void)
{
	platform_driver_unregister(&fih_apr_driver);
}
module_exit(fih_apr_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
