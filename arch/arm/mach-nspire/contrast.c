/*
 *  linux/arch/arm/mach-nspire/contrast.c
 *
 *  Copyright (C) 2012 Daniel Tang <tangrs@tangrs.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

#include <mach/nspire_mmio.h>

#define CONTRAST_PROCFS_NAME "backlight"

static struct proc_dir_entry *contrast_proc_entry;

static int contrast_read(char *buf, char **data, off_t offset, int len, int *eof, void *privdata) {
    len = snprintf(buf, PAGE_SIZE,
            "Screen backlight is %d.\n",
            readl(NSPIRE_APB_VIRTIO(NSPIRE_APB_CONTRAST + 0x20)));
    if (offset >= len) {
        *eof = 1;
        return 0;
    }
    memmove(buf, buf+offset, len-offset);
    return len-offset;
}

static int contrast_write(struct file *file, const char __user *buffer, unsigned long count, void *privdata )
{
    unsigned char val = 128;
    int ret;
    if ((ret = kstrtou8_from_user(buffer, count, 10, &val))) {
        return ret;
    }
    writel(val, NSPIRE_APB_VIRTIO(NSPIRE_APB_CONTRAST + 0x20));

    return count;
}

int contrast_procfs_init(void) {
    contrast_proc_entry = create_proc_entry(CONTRAST_PROCFS_NAME, 0644, NULL);
    if (!contrast_proc_entry) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       CONTRAST_PROCFS_NAME);
		return -ENOMEM;
	}

	contrast_proc_entry->read_proc  = contrast_read;
	contrast_proc_entry->write_proc = contrast_write;
    printk(KERN_INFO "Contrast settings mapped to /proc/%s\n", CONTRAST_PROCFS_NAME);
    return 0;
}