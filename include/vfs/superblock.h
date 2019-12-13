#ifndef NVMFS_SUPER_BLOCK_H
#define NVMFS_SUPER_BLOCK_H

#include <linux/fs.h>
#include <linux/slab.h>

extern const struct super_operations nvmfsSuperOps;

int NvmfsFillSuperBlock(struct super_block * sb, void * options, int silent);
void NvmfsKillSuperBlock(struct super_block * sb);

#endif