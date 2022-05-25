// SPDX-License-Identifier: GPL-2.0
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018  Redha Gouicem <redha.gouicem@lip6.fr>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include "ouichefs.h"

/*
 * Mount a ouiche_fs partition
 */

/* ouichefs_debug dentry */
static struct dentry *ouichefs_debug;
static struct dentry *subdir;

// function for handle read and write events
static ssize_t ouichefs_debug_read(struct file *f, char *buffer,
		size_t len, loff_t *offset)
{
	// /*recuperer le dentry de la partition en lui donnant le path */
	// int err;
	// struct path partition_path;

	// char *path_name = "/share/projet/ouichefs/ouichefsPartition";
	
	// err = kern_path(path_name, LOOKUP_FOLLOW, &partition_path);
	// if (err) {
	// 	return -1;
	// }

	// inode = d_backing_inode(path.dentry);

	return snprintf(buffer, PAGE_SIZE, KERN_INFO "hi from debugfs\n");
}
// static ssize_t ouichefs_debug_write(struct file *f, char *buffer,
// 		size_t len, loff_t *offset)
// {
// 	return 0;
// }


const struct file_operations ouichefs_file_fops = {
	.owner = THIS_MODULE,
	// .write = ouichefs_debug_write,
	.read = ouichefs_debug_read,
};

struct dentry *ouichefs_mount(struct file_system_type *fs_type, int flags,
			      const char *dev_name, void *data)
{
	struct dentry *dentry = NULL;

	dentry = mount_bdev(fs_type, flags, dev_name, data,
			    ouichefs_fill_super);
	if (IS_ERR(dentry))
		pr_err("'%s' mount failure\n", dev_name);
	else
		pr_info("'%s' mount success\n", dev_name);

	return dentry;
}

/*
 * Unmount a ouiche_fs partition
 */
void ouichefs_kill_sb(struct super_block *sb)
{
	kill_block_super(sb);

	pr_info("unmounted disk\n");
}

static struct file_system_type ouichefs_file_system_type = {
	.owner = THIS_MODULE,
	.name = "ouichefs",
	.mount = ouichefs_mount,
	.kill_sb = ouichefs_kill_sb,
	.fs_flags = FS_REQUIRES_DEV,
	.next = NULL,
};

static int __init ouichefs_init(void)
{
	int ret;

	ret = ouichefs_init_inode_cache();
	if (ret) {
		pr_err("inode cache creation failed\n");
		goto end;
	}

	ret = register_filesystem(&ouichefs_file_system_type);
	if (ret) {
		pr_err("register_filesystem() failed\n");
		goto end;
	}
	subdir = debugfs_create_dir("ouichefs_debug_dir", NULL);
	if (!subdir) {
		pr_err("debugfs_create_dir failed \n");
		ret = -ENOENT;
		goto end;
	}
		
	/* partie implantation de dbugfs */
	ouichefs_debug = debugfs_create_file("ouichefs_debug_file", 0644,
					subdir, NULL, &ouichefs_file_fops);
	if (!ouichefs_debug) {
		pr_err("debugfs_create_file ouichefs_debug failed \n");
		goto exit;
	}
		
	pr_info("module loaded\n");
exit:
	debugfs_remove_recursive(subdir);
	ret = -ENOENT;
end:
	return ret;
}

static void __exit ouichefs_exit(void)
{
	int ret;

	ret = unregister_filesystem(&ouichefs_file_system_type);
	if (ret)
		pr_err("unregister_filesystem() failed\n");

	ouichefs_destroy_inode_cache();
	debugfs_remove_recursive(subdir);
	pr_info("module unloaded\n");
}

module_init(ouichefs_init);
module_exit(ouichefs_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redha Gouicem, <redha.gouicem@lip6.fr>");
MODULE_DESCRIPTION("ouichefs, a simple educational filesystem for Linux");
