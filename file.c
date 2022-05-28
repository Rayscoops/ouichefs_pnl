// SPDX-License-Identifier: GPL-2.0
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018 Redha Gouicem <redha.gouicem@lip6.fr>
 */

#define pr_fmt(fmt) "ouichefs: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>

#include "ouichefs.h"
#include "bitmap.h"

/*
 * Map the buffer_head passed in argument with the iblock-th block of the file
 * represented by inode. If the requested block is not allocated and create is
 * true,  allocate a new block on disk and map it.
 */
static int ouichefs_file_get_block(struct inode *inode, sector_t iblock,
				   struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index;
	bool alloc = false;
	int ret = 0, bno;

	pr_info("[ouichefs_file_get_block]\n");
	/* If block number exceeds filesize, fail */
	if (iblock >= (OUICHEFS_BLOCK_SIZE >> 2) - 1)
		return -EFBIG;
	/* Read index block from disk */
	bh_index = sb_bread(sb, ci->index_block);
	if (!bh_index)
		return -EIO;
	index = (struct ouichefs_file_index_block *)bh_index->b_data;
	/*
	 * Check if iblock is already allocated. If not and create is true,
	 * allocate it. Else, get the physical block number.
	 */
	if (index->blocks[iblock] == 0) {
		if (!create)
			return 0;
		bno = get_free_block(sbi);
		if (!bno) {
			ret = -ENOSPC;
			goto brelse_index;
		}
		index->blocks[iblock] = bno;
		alloc = true;
	} else {
		bno = index->blocks[iblock];
	}
	/* Map the physical block to the given buffer_head */
	map_bh(bh_result, sb, bno);
brelse_index:
	brelse(bh_index);
	return ret;
}
/*
 * Called by the page cache to read a page from the physical disk and map it in
 * memory.
 */
static int ouichefs_readpage(struct file *file, struct page *page)
{
	pr_info("[ouichefs_readpage]\n");
	return mpage_readpage(page, ouichefs_file_get_block);
}

/*
 * Called by the page cache to write a dirty page to the physical disk (when
 * sync is called or when memory is needed).
 */
static int ouichefs_writepage(struct page *page, struct writeback_control *wbc)
{
	pr_info("[ouichefs_writepage]\n");
	return block_write_full_page(page, ouichefs_file_get_block, wbc);
}

/*
 * Called by the VFS when a write() syscall occurs on file before writing the
 * data in the page cache. This functions checks if the write will be able to
 * complete and allocates the necessary blocks through block_write_begin().
 */
static int ouichefs_write_begin(struct file *file,
				struct address_space *mapping, loff_t pos,
				unsigned int len, unsigned int flags,
				struct page **pagep, void **fsdata)
{

	//pr_info("Taille de len:%d | Taille de l'offset:%lld",len,pos);

	struct buffer_head *bh_new;
	struct buffer_head *bh_block;
	struct buffer_head *new_bh_block;
	struct inode *inode = file->f_inode;
	struct ouichefs_file_index_block *new_index;
	struct ouichefs_file_index_block *index;
	struct super_block *sb = inode->i_sb;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct buffer_head *bh_version;
	struct buffer_head *bh;
	struct ouichefs_file_index_block *index_version;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(file->f_inode->i_sb);
	int err, v, bno_version, i, block_number, k;
	uint32_t nr_allocs = 0;


	/* mise à jour de la latest_version et test si droit à l'ecriture */
	struct ouichefs_inode *cinode = NULL;
	pr_info("[ouichefs_write_begin]\n");

	/* Check if the write can be completed (enough space or have right?) */
	uint32_t inode_block = (inode->i_ino / OUICHEFS_INODES_PER_BLOCK) + 1;
	uint32_t inode_shift = inode->i_ino % OUICHEFS_INODES_PER_BLOCK;
	bh = sb_bread(sb, inode_block);
	if (!bh) {
		return -EIO;
	}
	cinode = (struct ouichefs_inode *)bh->b_data;

	if (pos + len > OUICHEFS_MAX_FILESIZE)
		return -ENOSPC;

	nr_allocs = max(pos + len, file->f_inode->i_size) / OUICHEFS_BLOCK_SIZE;
	if (nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;
	if (nr_allocs > sbi->nr_free_blocks)
		return -ENOSPC;



///////////////////////////////////////////////////////////////////////////////

	bh_new = sb_bread(sb, ci->index_block);
	if (!bh_new)
		return -EIO;

	/* récupère le pointeur vers le début des datas du block */
	index = (struct ouichefs_file_index_block *)bh_new->b_data;

	if (index->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] == 0) {
		pr_info("/* premiere fois qu'on ecrit sur le file */\n");
		index->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] = -1;
		cinode->can_write = true;
		cinode->nb_versions = 0;
		mark_buffer_dirty(bh_new);
		sync_dirty_buffer(bh_new);
	} else {
		if (!cinode->can_write) {
			pr_err("Read-only file system\n");
			return -EROFS;
		}
		/* récupère un block free */
		bno_version = get_free_block(sbi);
		bh_new = sb_bread(sb, bno_version);
		if (!bh_new) {
			put_block(sbi, bno_version);
			return -EIO;
		}

		new_index = (struct ouichefs_file_index_block *)bh_new->b_data;
		k = 0;
		/* Pour chaque blocs alloués on copie les données */
		/*while (index->blocks[k] != 0) {
			block_number = get_free_block(sbi);
			new_bh_block = sb_bread(sb, block_number);
			bh_block = sb_bread(sb, index->blocks[i]);
			if (!bh_block)
				return -EIO;
			memcpy(new_bh_block->b_data, bh_block->b_data, OUICHEFS_BLOCK_SIZE);
			pr_info("[DATA %d : %s]", block_number, new_bh_block->b_data);
			mark_buffer_dirty(new_bh_block);
			sync_dirty_buffer(new_bh_block);
			k++;
		}*/
		/* Ajout du numéro de blocs contenant l'ancienne version */
		new_index->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] = ci->index_block;
		/* les données de la nouvelle version */
		ci->index_block = bno_version;
		mark_buffer_dirty(bh_new);
		sync_dirty_buffer(bh_new);
	}

	//struct ouichefs_inode_info *ci = NULL;

	ci = OUICHEFS_INODE(inode);

	cinode += inode_shift;

	cinode->last_index_block = ci->index_block;
	cinode->nb_versions++; /* on viens de write on incrémente */
	mark_inode_dirty(inode);
	pr_info("La dernière version est :%d\n",cinode->last_index_block);
	/* Affichage du nombre de version */
	v = 1;
	bh_version = sb_bread(sb, ci->index_block);
	index_version = (struct ouichefs_file_index_block *)bh_version->b_data;
	pr_info("Dernier block :%d\n", index_version->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1]);
	int q = 0;

	struct buffer_head *bh_data;
	while (index_version->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] != -1) {
/*		for(q = 0; q < index_version->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1] ; q++){
			bh_data = sb_bread(sb, index_version->blocks[q]);
			pr_info("block:%d | %s\n",q,bh_data->b_data);
		}*/
		bh_version = sb_bread(sb, index_version->blocks[(OUICHEFS_BLOCK_SIZE >> 2) - 1]);
		index_version = (struct ouichefs_file_index_block *)bh_version->b_data;
		v++;
	}
	pr_info("[%d version(s) du fichier]\n", v);
//////////////////////////////////////////////////////////////////////////////////////
	/* prepare the write 	*/
	err = block_write_begin(mapping, pos, len, flags, pagep,
				ouichefs_file_get_block);
	/* if this failed, reclaim newly allocated blocks */
	if (err < 0) {
		pr_err("%s:%d: newly allocated blocks reclaim not implemented yet\n",
		       __func__, __LINE__);
	}
	return err;

}

/*
 * Called by the VFS after writing data from a write() syscall to the page
 * cache. This functions updates inode metadata and truncates the file if
 * necessary.
 */
static int ouichefs_write_end(struct file *file, struct address_space *mapping,
			      loff_t pos, unsigned int len, unsigned int copied,
			      struct page *page, void *fsdata)
{
	int ret;
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh_data;
	int block_data;
	/* Complete the write() */
	/*pr_info("Taille de len:%d | Taille de l'offset:%lld",len,pos);
	pr_info("copied:%d\n",copied);*/
	pr_info("Taille inode:%lld\n",inode->i_size);
	ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);

	if (ret < len) {
		pr_err("%s:%d: wrote less than asked... what do I do? nothing for now...\n",
		       __func__, __LINE__);
	} else {
		uint32_t nr_blocks_old = inode->i_blocks;
		int k;
		/* Update inode metadata */
		inode->i_blocks = inode->i_size / OUICHEFS_BLOCK_SIZE + 2;
		inode->i_mtime = inode->i_ctime = current_time(inode);
		mark_inode_dirty(inode);
		
		void *addr = kmap(page);
		pr_info("index:%ld\n",page->index);
		k = 0;
		/* si je dois créer une nouvelle version */
		if(page->index != 0){
			pr_info("Pas besoin de nouvelle version");
			/* récupère l'avnt dernière version courante */
			bh_data = sb_bread(sb, block_data);
			if (!bh_data)
				return -EIO;
			memcpy(new_bh_block->b_data, bh_block->b_data, OUICHEFS_BLOCK_SIZE);
			
		}
/*		while (index->blocks[k] != 0) {
			
			
			memcpy(new_bh_block->b_data, bh_block->b_data, OUICHEFS_BLOCK_SIZE);
			pr_info("[DATA %d : %s]", block_number, new_bh_block->b_data);
			mark_buffer_dirty(new_bh_block);
			sync_dirty_buffer(new_bh_block);
			k++;
		}*/





		/* libère les blocks de la page */
		kunmap(page);
		






		/* If file is smaller than before, free unused blocks */
		if (nr_blocks_old > inode->i_blocks) {
			int i;
			struct buffer_head *bh_index;
			struct ouichefs_file_index_block *index;

			/* Free unused blocks from page cache */
			truncate_pagecache(inode, inode->i_size);

			/* Read index block to remove unused blocks */
			bh_index = sb_bread(sb, ci->index_block);
			if (!bh_index) {
				pr_err("failed truncating '%s'. we just lost %llu blocks\n",
				       file->f_path.dentry->d_name.name,
				       nr_blocks_old - inode->i_blocks);
				goto end;
			}
			index = (struct ouichefs_file_index_block *)
				bh_index->b_data;

			for (i = inode->i_blocks - 1; i < nr_blocks_old - 1;
			     i++) {
				put_block(OUICHEFS_SB(sb), index->blocks[i]);
				index->blocks[i] = 0;
			}
			mark_buffer_dirty(bh_index);
			brelse(bh_index);
		}
	}
end:
	return ret;
}

const struct address_space_operations ouichefs_aops = {
	.readpage    = ouichefs_readpage,
	.writepage   = ouichefs_writepage,
	.write_begin = ouichefs_write_begin,
	.write_end   = ouichefs_write_end
};

const struct file_operations ouichefs_file_ops = {
	.owner      = THIS_MODULE,
	.llseek     = generic_file_llseek,
	.read_iter  = generic_file_read_iter,
	.write_iter = generic_file_write_iter
};