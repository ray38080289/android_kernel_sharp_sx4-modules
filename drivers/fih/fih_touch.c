#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "fih_touch.h"

#define FIH_PROC_DIR                    "AllHWList"
#define FIH_PROC_TP_SELF_TEST           "AllHWList/tp_self_test"
#define FIH_PROC_TP_IC_FW_VER           "AllHWList/tp_fw_ver"
#define FIH_PROC_TP_UPGRADE             "AllHWList/tp_upgrade"
#define FIH_PROC_TP_VENDOR              "AllHWList/tp_vendor"

struct fih_touch_cb touch_cb = {
    .touch_selftest = NULL,
    .touch_selftest_result = NULL,
    .touch_tpfwver_read = NULL,
    .touch_fwupgrade = NULL,
    .touch_fwupgrade_read = NULL,
    .touch_vendor_read = NULL,
};
EXPORT_SYMBOL(touch_cb);
//touch_vendor start
static int fih_touch_read_vendor_show(struct seq_file *m, void *v)
{
	char vendor[30]={0};
	if (touch_cb.touch_vendor_read != NULL)
	{
		pr_info("F@Touch Read Touch Vendor\n");
		touch_cb.touch_vendor_read(vendor);
	seq_printf(m, "%s", vendor);
	}
	return 0;
}

static int fih_touch_vendor_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_touch_read_vendor_show, NULL);
};

static struct proc_ops touch_vendor_proc_file_ops = {
	.proc_open    = fih_touch_vendor_proc_open,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release
};
//touch_vendor end

//touch self test  start
static int fih_touch_self_test_show(struct seq_file *m, void *v)
{
    if (touch_cb.touch_selftest_result != NULL)
    {
        pr_info("F@Touch Touch Selftest Result Read\n");
        seq_printf(m, "%d\n", touch_cb.touch_selftest_result());
    }
    return 0;
}

static int fih_touch_self_test_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fih_touch_self_test_show, NULL);
};

static ssize_t fih_touch_self_test_proc_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *ppos)
{
    if(touch_cb.touch_selftest != NULL)
    {
        pr_info("F@Touch Do Touch Selftest\n");
        touch_cb.touch_selftest();
    } else {
        pr_info("F@Touch Selftest not found\n");
    }
    return count;
}

static struct proc_ops touch_self_test_proc_file_ops = {
    .proc_write   = fih_touch_self_test_proc_write,
    .proc_open    = fih_touch_self_test_proc_open,
    .proc_read    = seq_read,
    .proc_lseek  = seq_lseek,
    .proc_release = single_release
};
//touch self test  end

//touch_fwver start
static int fih_touch_read_fwver_show(struct seq_file *m, void *v)
{
    char fwver[30]={0};

    if(touch_cb.touch_tpfwver_read != NULL)
    {
        pr_info("F@Touch Read Touch Firmware Version\n");
        touch_cb.touch_tpfwver_read(fwver);
        seq_printf(m, "%s", fwver);
    }
    return 0;
}

static int fih_touch_fwver_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fih_touch_read_fwver_show, NULL);
};

static struct proc_ops touch_fwver_proc_file_ops = {
    .proc_open    = fih_touch_fwver_proc_open,
    .proc_read    = seq_read,
    .proc_lseek  = seq_lseek,
    .proc_release = single_release
};
//touch_fwver end

//touch_upgrade start
static int fih_touch_upgrade_read_show(struct seq_file *m, void *v)
{
    char upgrade_flag[10]={0};

    if(touch_cb.touch_fwupgrade_read != NULL)
    {
        pr_info("F@Touch Read Touch Upgrade Flag\n");
        touch_cb.touch_fwupgrade_read(upgrade_flag);
        seq_printf(m, "%s", upgrade_flag);
    }
    return 0;
}

static int fih_touch_upgrade_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fih_touch_upgrade_read_show, NULL);
};

static ssize_t fih_touch_upgrade_proc_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *ppos)
{
    char *buf;
    unsigned int input = 0;

    if (touch_cb.touch_fwupgrade == NULL)
        return -EINVAL;

    if (count < 1)
        return -EINVAL;

    buf = kzalloc(count, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;
    input = simple_strtoul(buf, NULL, 10);

    if (touch_cb.touch_fwupgrade != NULL)
    {
        pr_info("F@Touch FW upgrade\n");
        touch_cb.touch_fwupgrade(input);
    }

    kfree(buf);
    /*int input;

    if (sscanf(buffer, "%u", &input) != 1)
    {
         return -EINVAL;
    }

    if(touch_cb.touch_fwupgrade != NULL)
    {
        pr_info("F@Touch Write Touch Upgrade(%d)\n", input);
        touch_cb.touch_fwupgrade(input);
    }*/
    return count;
}

static struct proc_ops touch_upgrade_proc_file_ops = {
    .proc_write   = fih_touch_upgrade_proc_write,
    .proc_open    = fih_touch_upgrade_proc_open,
    .proc_read    = seq_read,
    .proc_lseek  = seq_lseek,
    .proc_release = single_release
};
//touch_upgrade end

static int __init fih_touch_init(void)
{
    //pr_err("touch probe success, create proc file\n");
    proc_mkdir(FIH_PROC_DIR, NULL);  //avoid warning when re-create folder

    //F@Touch Self Test
    if (proc_create(FIH_PROC_TP_SELF_TEST, 0, NULL, &touch_self_test_proc_file_ops) == NULL)
    {
        pr_err("fail to create proc/%s\n", FIH_PROC_TP_SELF_TEST);
        return (1);
    }

    //F@Touch Read IC's firmware version
    if (proc_create(FIH_PROC_TP_IC_FW_VER, 0, NULL, &touch_fwver_proc_file_ops) == NULL)
    {
        pr_err("fail to create proc/%s\n", FIH_PROC_TP_IC_FW_VER);
        return (1);
    }

    //F@Touch Firmware Upgrade
    if (proc_create(FIH_PROC_TP_UPGRADE, 0, NULL, &touch_upgrade_proc_file_ops) == NULL)
    {
        pr_err("fail to create proc/%s\n", FIH_PROC_TP_UPGRADE);
        return (1);
    }

    //F@Touch Get Vendor name
    if (proc_create(FIH_PROC_TP_VENDOR, 0, NULL, &touch_vendor_proc_file_ops) == NULL)
    {
        pr_err("fail to create proc/%s\n", FIH_PROC_TP_VENDOR);
        return (1);
    }

    return (0);
}

static void __exit fih_touch_exit(void)
{
    remove_proc_entry(FIH_PROC_TP_SELF_TEST, NULL);
    remove_proc_entry(FIH_PROC_TP_IC_FW_VER, NULL);
    remove_proc_entry(FIH_PROC_TP_UPGRADE, NULL);
}

module_init(fih_touch_init);
module_exit(fih_touch_exit);

MODULE_LICENSE("GPL");
