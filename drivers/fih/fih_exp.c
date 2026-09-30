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

/* reference /proc/pl_lk
 * kernel_device_modules-6.6/drivers/misc/mediatek/log_store/log_store.c
 * remap_lowmem()
 * log_store_late_init()
 */

static unsigned int fih_exp_addr = 0x9AE00000;
static unsigned int fih_exp_size = 0x00200000;
static char *fih_exp_pbuf = NULL;

static struct proc_dir_entry *entry_log = NULL;

static void *remap_lowmem(phys_addr_t start, phys_addr_t size)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int page_count;
	pgprot_t prot = PAGE_KERNEL;
	unsigned int i;
	void *vaddr;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	prot = pgprot_writecombine(PAGE_KERNEL);
	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < page_count; i++) {
		phys_addr_t addr = page_start + i * PAGE_SIZE;
		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	vaddr = vmap(pages, page_count, VM_MAP, prot);
	kfree(pages);
	if (!vaddr) {
		pr_notice("%s: Failed to map %u pages\n", __func__, page_count);
		return NULL;
	}

	return vaddr + offset_in_page(start);
}

static int fih_exp_proc_read(struct seq_file *m, void *v)
{
	if (fih_exp_pbuf == NULL) {
		seq_puts(m, "log_pbuf is NULL.\n");
		return 0;
	}

	seq_write(m, fih_exp_pbuf, fih_exp_size);
	return 0;
}

static int fih_exp_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, fih_exp_proc_read, inode->i_private);
};

static struct proc_ops fih_exp_fops = {
	.proc_open    = fih_exp_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release
};

static int __init fih_exp_init(void)
{
	fih_exp_pbuf = remap_lowmem(fih_exp_addr, fih_exp_size);
	if (fih_exp_pbuf == NULL) {
		pr_err("%s: failed to remap_lowmem\n", __func__);
		return 0;
	}

	entry_log = proc_create("pl_lk_exp", 0444, NULL, &fih_exp_fops);
	if (!entry_log) {
		pr_err("%s: failed to create proc pl_lk_exp\n", __func__);
	}

	return 0;
}
module_init(fih_exp_init);

static void __exit fih_exp_exit(void)
{
	if (entry_log) proc_remove(entry_log);
}
module_exit(fih_exp_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
