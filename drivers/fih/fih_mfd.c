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

static char pid[64] = {0};
static char bt_mac[64] = {0};
static char wifi_mac[64] = {0};
static char imei_1[64] = {0};
static char imei_2[64] = {0};

static struct proc_dir_entry *entry_pid = NULL;
static struct proc_dir_entry *entry_bt_mac = NULL;
static struct proc_dir_entry *entry_wifi_mac = NULL;
static struct proc_dir_entry *entry_imei_1 = NULL;
static struct proc_dir_entry *entry_imei_2 = NULL;

static int fih_mfd_proc_read_pid(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", pid);
	return 0;
}

static int fih_mfd_proc_open_pid(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_mfd_proc_read_pid, inode->i_private);
}

static const struct proc_ops fih_mfd_fops_pid = {
	.proc_open    = fih_mfd_proc_open_pid,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_mfd_proc_read_bt_mac(struct seq_file *m, void *v)
{
	char tmp[(sizeof(bt_mac) * 2)];
	unsigned int i, k;

	/* 0123456789AB -> 01:23:45:67:89:AB */
	memset(tmp, 0, sizeof(tmp));
	k = 0;
	for (i = 0; i < strlen(bt_mac); i++) {
		if ((i > 0)&&((i % 2) == 0)) tmp[k++] = ':';
		tmp[k++] = bt_mac[i];
	}

	seq_printf(m, "%s\n", tmp);

	return 0;
}

static int fih_mfd_proc_open_bt_mac(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_mfd_proc_read_bt_mac, inode->i_private);
}

static const struct proc_ops fih_mfd_fops_bt_mac = {
	.proc_open    = fih_mfd_proc_open_bt_mac,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_mfd_proc_read_wifi_mac(struct seq_file *m, void *v)
{
	char tmp[(sizeof(wifi_mac) * 2)];
	unsigned int i, k;

	/* 0123456789AB -> 01:23:45:67:89:AB */
	memset(tmp, 0, sizeof(tmp));
	k = 0;
	for (i = 0; i < strlen(wifi_mac); i++) {
		if ((i > 0)&&((i % 2) == 0)) tmp[k++] = ':';
		tmp[k++] = wifi_mac[i];
	}

	seq_printf(m, "%s\n", tmp);

	return 0;
}

static int fih_mfd_proc_open_wifi_mac(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_mfd_proc_read_wifi_mac, inode->i_private);
}

static const struct proc_ops fih_mfd_fops_wifi_mac = {
	.proc_open    = fih_mfd_proc_open_wifi_mac,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_mfd_proc_read_imei_1(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", imei_1);
	return 0;
}

static int fih_mfd_proc_open_imei_1(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_mfd_proc_read_imei_1, inode->i_private);
}

static const struct proc_ops fih_mfd_fops_imei_1 = {
	.proc_open    = fih_mfd_proc_open_imei_1,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_mfd_proc_read_imei_2(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", imei_2);
	return 0;
}

static int fih_mfd_proc_open_imei_2(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_mfd_proc_read_imei_2, inode->i_private);
}

static const struct proc_ops fih_mfd_fops_imei_2 = {
	.proc_open    = fih_mfd_proc_open_imei_2,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int fih_mfd_property(struct platform_device *pdev)
{
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,pid", NULL);
	if (!p_chr) {
		pr_err("%s: pid not specified\n", __func__);
	} else {
		memset(pid, 0, sizeof(pid));
		snprintf(pid, sizeof(pid), "%s\n", p_chr);
	}
	pr_info("%s: productid = (%s)\n", __func__, pid);

	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,bt_mac", NULL);
	if (!p_chr) {
		pr_err("%s: bt_mac not specified\n", __func__);
	} else {
		memset(bt_mac, 0, sizeof(bt_mac));
		strlcpy(bt_mac, p_chr, sizeof(bt_mac));
	}
	pr_info("%s: bt_mac = (%s)\n", __func__, bt_mac);

	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,wifi_mac", NULL);
	if (!p_chr) {
		pr_err("%s: wifi_mac not specified\n", __func__);
	} else {
		memset(wifi_mac, 0, sizeof(wifi_mac));
		strlcpy(wifi_mac, p_chr, sizeof(wifi_mac));
	}
	pr_info("%s: wifi_mac = (%s)\n", __func__, wifi_mac);

	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,imei_1", NULL);
	if (!p_chr) {
		pr_err("%s: imei_1 not specified\n", __func__);
	} else {
		memset(imei_1, 0, sizeof(imei_1));
		strlcpy(imei_1, p_chr, sizeof(imei_1));
	}
	pr_info("%s: imei_1 = (%s)\n", __func__, imei_1);

	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,imei_2", NULL);
	if (!p_chr) {
		pr_err("%s: imei_2 not specified\n", __func__);
	} else {
		memset(imei_2, 0, sizeof(imei_2));
		strlcpy(imei_2, p_chr, sizeof(imei_2));
	}
	pr_info("%s: imei_2 = (%s)\n", __func__, imei_2);

	return 0;
}

static int fih_mfd_probe(struct platform_device *pdev)
{
	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -EINVAL;
	}

	fih_mfd_property(pdev);

	entry_pid = proc_create("productid", 0444, NULL, &fih_mfd_fops_pid);
	if (!entry_pid) {
		pr_err("%s: failed to create proc productid\n", __func__);
	}

	entry_bt_mac = proc_create("bt_mac", 0444, NULL, &fih_mfd_fops_bt_mac);
	if (!entry_bt_mac) {
		pr_err("%s: failed to create proc bt_mac\n", __func__);
	}

	entry_wifi_mac = proc_create("wifi_mac", 0444, NULL, &fih_mfd_fops_wifi_mac);
	if (!entry_wifi_mac) {
		pr_err("%s: failed to create proc wifi_mac\n", __func__);
	}

	entry_imei_1 = proc_create("imei", 0444, NULL, &fih_mfd_fops_imei_1);
	if (!entry_imei_1) {
		pr_err("%s: failed to create proc imei\n", __func__);
	}

	entry_imei_2 = proc_create("imei2", 0444, NULL, &fih_mfd_fops_imei_2);
	if (!entry_imei_2) {
		pr_err("%s: failed to create proc imei2\n", __func__);
	}

	return 0;
}

static int fih_mfd_remove(struct platform_device *pdev)
{
	if (entry_imei_2) proc_remove(entry_imei_2);
	if (entry_imei_1) proc_remove(entry_imei_1);
	if (entry_wifi_mac) proc_remove(entry_wifi_mac);
	if (entry_bt_mac) proc_remove(entry_bt_mac);
	if (entry_pid) proc_remove(entry_pid);

	return 0;
}

static void fih_mfd_shutdown(struct platform_device *pdev)
{
	//
}

static const struct of_device_id fih_mfd_dt_match[] = {
	{ .compatible = "fih_mfd" },
	{}
};

static struct platform_driver fih_mfd_driver = {
	.probe = fih_mfd_probe,
	.remove = fih_mfd_remove,
	.shutdown = fih_mfd_shutdown,
	.driver = {
		.name = "fih_mfd",
		.owner = THIS_MODULE,
		.of_match_table = fih_mfd_dt_match,
	},
};

static int __init fih_mfd_init(void)
{
	return platform_driver_register(&fih_mfd_driver);
}
module_init(fih_mfd_init);

static void __exit fih_mfd_exit(void)
{
	platform_driver_unregister(&fih_mfd_driver);
}
module_exit(fih_mfd_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
