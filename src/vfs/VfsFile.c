#include "DirInodeInfo.h"
#include "File.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include <linux/fs.h>

static ssize_t NvmfsFileRead(struct file * file, char __user * buffer, size_t size, loff_t * offset)
{
    struct FileInodeInfo * inodeInfo;
    struct NvmfsInfo * info;
    ssize_t readSize;

    inodeInfo = file->f_inode->i_private;
    info = file->f_inode->i_sb->s_fs_info;
    readSize = FileInodeInfoReadData(inodeInfo, buffer, size, *offset, &info->acc);
    if (readSize < 0)
        return readSize;
    (*offset) += readSize;
    return readSize;
}

static ssize_t NvmfsFileWrite(struct file * file, const char __user * buffer, size_t size, loff_t * offset)
{
    struct FileInodeInfo * inodeInfo;
    struct NvmfsInfo * info;
    ssize_t writeSize;

    inodeInfo = file->f_inode->i_private;
    info = file->f_inode->i_sb->s_fs_info;
    writeSize = FileInodeInfoWriteData(inodeInfo, (void *)buffer, size, *offset, &info->acc);
    if (writeSize < 0)
        return writeSize;
    (*offset) += writeSize;
    return writeSize;
}

static UINT8 TypeToVfsType(UINT8 type)
{
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        return DT_REG;
    case INODE_TYPE_DIR_FILE:
        return DT_DIR;
    case INODE_TYPE_LINK_FILE:
        return DT_LNK;
    }
    return DT_UNKNOWN;
}

static int NvmfsIterateFunc(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino)
{
    struct dir_context * context;
    int err;

    context = data;
    err = dir_emit(context, name, len, ino, TypeToVfsType(type));
    context->pos++;
    return !err;
}

static int NvmfsIterate(struct file * file, struct dir_context * context)
{
    struct DirInodeInfo * dirInfo;
    struct NvmfsInfo * fsInfo;

    dirInfo = file->f_inode->i_private;
    fsInfo = file->f_inode->i_sb->s_fs_info;
    DirInodeInfoIterateDentry(dirInfo, NvmfsIterateFunc, context, &fsInfo->acc);
    return 0;
}

struct file_operations NvmfsDirFileOps = {.iterate = NvmfsIterate};
struct file_operations NvmfsRegFileOps = {.read = NvmfsFileRead, .write = NvmfsFileWrite};

// struct file_operations
// {
//     ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
//     ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
//     int (*open)(struct inode *, struct file *);
//     int (*release)(struct inode *, struct file *);
//     int (*iterate)(struct file *, struct dir_context *)
// }