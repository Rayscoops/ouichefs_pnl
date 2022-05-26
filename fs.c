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

#include <linux/seq_file.h>
#include <linux/buffer_head.h>

#include "ouichefs.h"

/*
 * Mount a ouiche_fs partition
 */


/* 
 * cette fonction est appelé lorsque on essai de lire le 
 * debugs fs de la partition ouichefs elle affiche :
 * une colonne avec le numéro d'inode
 * ue colonne avec le numéro de version
 * une colonne avec une liste des numéros de blocs d'index 
 * correspondant à l'historique du fichier
 */
static int debug_ouichefs_show(struct seq_file *s_file, void *v)
{
	pr_info("[debug_ouichefs_show]\n");
	/*
	l'inode du fichier du dbugfs
	struct inode *pfile_inode = s_file->file->f_inode;
	*/
	/* recupérer le super block de la partition*/
	struct super_block *sb= (struct super_block *)s_file->file->f_inode->i_private;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	/* parcourir les inodes de la partition */
	struct inode *sub_file_inode;
	int num_inode;
	struct ouichefs_inode_info *ci;
	struct buffer_head *bh_index;
	struct ouichefs_file_index_block *index;
	int nb_versions;
	/* sert pour passer d'une version a une autre */
	uint32_t cur_v, last_v;
	int taille_max;
	taille_max = 2000;
	char msg[taille_max];
	
	for (num_inode = 0; num_inode < sbi->nr_inodes; num_inode++) {
		memset(msg, 0, taille_max * (sizeof(char)));
		sub_file_inode = ouichefs_iget(sb, num_inode);

		/*soit l'inode n'est pas allouée 
		soit le fichier n'est pas régulier*/
		if((sub_file_inode->i_nlink == 0)\
		|| !S_ISREG(sub_file_inode->i_mode)) {
			continue;	
		}
		/* j'ai trouvé un file */
		ci = OUICHEFS_INODE(sub_file_inode);
		bh_index = sb_bread(sb, ci->index_block);
		if (!bh_index) {
			seq_printf(s_file, "errer lors de \
					la récupération des données \n");
			return 0;
		}

		index = (struct ouichefs_file_index_block *)bh_index->b_data;
		nb_versions = 1;
		cur_v = ci->index_block;
		int offset = taille_max;
        	offset -= scnprintf(msg + (taille_max - offset), offset,"%d", cur_v);
		while (index->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] != -1) {
			last_v = index->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1];
			
			bh_index = sb_bread(sb, last_v);
			if (!bh_index) {
				seq_printf(s_file, "errer lors de \
					la récupération des données \n");
				return 0;
			}
			index = (struct ouichefs_file_index_block *)bh_index->b_data;
			cur_v = last_v;
			nb_versions++;
			offset -= scnprintf(msg + (taille_max - offset), offset,",%d", cur_v);
		}
		seq_printf(s_file, "inode:%ld | nombre de version:%d | liste des blocks de version:{%s}\n",
				sub_file_inode->i_ino,
				nb_versions,
				msg);
		
		iput(sub_file_inode);
	}
	return 0;
}

/* 
 * cette fonction est appelé a l'ouverture du debugfs
 * de la partition 
 */
static int debug_ouichefs_open(struct inode *inode, struct file *file)
{
	pr_info("[debug_ouichefs_open]\n");
	return single_open(file, debug_ouichefs_show, NULL);
}


const struct file_operations debug_ouichefs_ops = {
	.owner = THIS_MODULE,
	.open = debug_ouichefs_open,
	.read  = seq_read,
	.release = single_release,
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

	/* creation de debugfs */
	/*-------------------------------------------------------------*/
	struct super_block *sb = dentry->d_inode->i_sb;
	struct dentry *ouichefs_debug;
	/* recupération du super block de l'inode de la partition */
	
	ouichefs_debug = debugfs_create_file("ouichefs_debug_file", 0600,
					NULL, NULL, &debug_ouichefs_ops);
	if (!ouichefs_debug) {
		pr_err("debugfs_create_file ouichefs_debug failed \n");
		goto err;
	}

	/* stocker le super block pour le recupérer lors du show */
	ouichefs_debug->d_inode->i_private = sb;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	/* stocker le file qu'il faut consulter dans les info de la partition */
	sbi->ouichefs_debug_file = ouichefs_debug;
	mark_inode_dirty(ouichefs_debug->d_inode);
	pr_info("module loaded\n");
	/*-------------------------------------------------------------*/
	return dentry;
err:
	return -ENOENT;
}

/*
 * Unmount a ouiche_fs partition
 */
void ouichefs_kill_sb(struct super_block *sb)
{
	/* lors de la supression de la partition penser a remouve le debugfs*/
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	debugfs_remove(sbi->ouichefs_debug_file);
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
	pr_info("module loaded\n");

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
	pr_info("module unloaded\n");
}

module_init(ouichefs_init);
module_exit(ouichefs_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redha Gouicem, <redha.gouicem@lip6.fr>");
MODULE_DESCRIPTION("ouichefs, a simple educational filesystem for Linux");