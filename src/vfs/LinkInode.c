#include "LinkInode.h"
#include "inode.h"

int nvmfs_symlink(struct inode * pNodeDir, struct dentry * pDentry, const char * symname)
{
    int err;
    struct inode * pInode = nvmfs_alloc_inode(pNodeDir->i_sb, pNodeDir, S_IFLNK | S_IRWXUGO, 0);
    if (pInode)
    {
        // copy the symname to pagecache
        err = page_symlink(pInode, symname, strlen(symname) + 1);
        if (!err)
        {
            d_instantiate(pDentry, pInode);
            dget(pDentry);

            pNodeDir->i_mtime = pNodeDir->i_ctime = current_time(pNodeDir);
        }
        else
            iput(pInode);

        return err;
    }

    return -ENOSPC;
}