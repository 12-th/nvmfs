#include "nvmfs.h"
#include "superblock.h"
#include <linux/module.h>
#include <linux/slab.h>

// mount superblock, will finally call demofs_fill_sb
struct dentry * nvmfsMount(struct file_system_type * pFSType, int flags, const char * devName, void * options)
{
    // use mount_nodev because our file system is not on physical block device
    return mount_nodev(pFSType, flags, options, NvmfsFillSuperBlock);
}

struct file_system_type nvmfs = {
    .owner = THIS_MODULE, .name = "nvmfs", .mount = nvmfsMount, .kill_sb = NvmfsKillSuperBlock};

// register filesystem
static int __init init_nvmfs(void)
{
    return register_filesystem(&nvmfs);
}

static void __exit clean_nvmfs(void)
{
    unregister_filesystem(&nvmfs);
}

MODULE_LICENSE("GPL");

module_init(init_nvmfs);
module_exit(clean_nvmfs);