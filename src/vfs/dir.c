#include "DirInode.h"
#include "LinkInode.h"
#include "inode.h"
#include <linux/fs.h>

static int nvmfs_create(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, bool excl)
{
    return nvmfs_mknod(pDirNode, pDentry, mode | S_IFREG, 0);
}

static int nvmfs_mkdir(struct inode * pDirNode, struct dentry * pDentry, umode_t mode)
{
    int res = nvmfs_mknod(pDirNode, pDentry, mode | S_IFDIR, 0);

    // increase parent dir inode's reference count
    if (!res)
        inc_nlink(pDirNode);

    return res;
}

const struct inode_operations nvmfsDirInodeOps = {.create = nvmfs_create,
                                                  .lookup = simple_lookup,
                                                  .link = simple_link,
                                                  .unlink = simple_unlink,
                                                  .symlink = nvmfs_symlink,
                                                  .mkdir = nvmfs_mkdir,
                                                  .rmdir = simple_rmdir,
                                                  .mknod = nvmfs_mknod,
                                                  .rename = simple_rename};

// void NvmfsDirInodeAddNewFile(struct inode * vfsDirInode, struct dentry * vfsFileDentry, struct inode * vfsFileInode)
// {
//     struct NvmfsInode *dirInode, *fileInode;
//     struct DirInodeNewFileLogEntry * entry;
//     UINT64 entrySize;
//     struct NvmfsInfo * info;

//     info = vfsDirInode->i_sb->s_fs_info;
//     dirInode = container_of(vfsDirInode, struct NvmfsInode, vfsInode);
//     fileInode = container_of(vfsFileDentry, struct NvmfsInode, vfsInode);
//     entrySize = sizeof(struct DirInodeNewFileLogEntry) + vfsFileDentry->d_name.len;
//     entry = kmalloc(entrySize, GFP_KERNEL);
//     entry->ino = fileInode->ino;
//     entry->size = vfsFileDentry->d_name.len;
//     memcpy(entry->name, vfsFileDentry->d_name.name, entry->size);
//     LogWrite(&dirInode->log, entrySize, entry, DIR_INODE_NEW_FILE_LOG, NULL, &info->inodePool, &info->acc);
// }
