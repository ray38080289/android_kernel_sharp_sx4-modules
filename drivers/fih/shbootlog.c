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

static unsigned int shbootlog_addr = 0x9b000000;
static unsigned int shbootlog_size = 0x00400000;
static char *shbootlog_pbuf = NULL;

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

static int shbootlog_proc_read(struct seq_file *m, void *v)
{
	if (shbootlog_pbuf == NULL) {
		seq_puts(m, "log_pbuf is NULL.\n");
		return 0;
	}

	seq_write(m, shbootlog_pbuf, shbootlog_size);
	return 0;
}

static int shbootlog_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, shbootlog_proc_read, inode->i_private);
};

static struct proc_ops shbootlog_fops = {
	.proc_open    = shbootlog_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release
};

static int __init shbootlog_init(void)
{
	shbootlog_pbuf = remap_lowmem(shbootlog_addr, shbootlog_size);
	if (shbootlog_pbuf == NULL) {
		pr_err("%s: failed to remap_lowmem\n", __func__);
		return 0;
	}

	entry_log = proc_create("shbootlog", 0444, NULL, &shbootlog_fops);
	if (!entry_log) {
		pr_err("%s: failed to create proc shbootlog\n", __func__);
	}

	return 0;
}
module_init(shbootlog_init);

static void __exit shbootlog_exit(void)
{
	if (entry_log) proc_remove(entry_log);
}
module_exit(shbootlog_exit);

MODULE_DESCRIPTION("FIH driver");
MODULE_AUTHOR("FIH author");
MODULE_LICENSE("GPL");
