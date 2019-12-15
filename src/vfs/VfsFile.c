#include "DirInodeInfo.h"
#include "File.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "InodeType.h"
#include "common.h"
#include <linux/fs.h>

static int NvmfsFileOpen(struct inode * inode, struct file * file)
{
    // DEBUG_PRINT("file open, ino is %ld", ((struct BaseInodeInfo *)(inode->i_private))->thisIno);
    DEBUG_PRINT("file open");
    return 0;
}

static int NvmfsFileRelease(struct inode * inode, struct file * file)
{
    // DEBUG_PRINT("file close, ino is %ld", ((struct BaseInodeInfo *)(inode->i_private))->thisIno);
    DEBUG_PRINT("file close");
    return 0;
}

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

    // struct FileInodeInfo * inodeInfo;
    // inodeInfo = file->f_inode->i_private;
    // FileInodePrintInfo(inodeInfo);
    // return size;
}

static int NvmfsIterateFunc(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino)
{
    struct dir_context * context;
    int err;

    context = data;
    err = dir_emit(context, name, len, ino, TypeToVfsDentryType(type));
    context->pos++;
    return !err;
}

static int NvmfsIterate(struct file * file, struct dir_context * context)
{
    struct DirInodeInfo * dirInfo;
    struct NvmfsInfo * fsInfo;

    dirInfo = file->f_inode->i_private;
    fsInfo = file->f_inode->i_sb->s_fs_info;
    DEBUG_PRINT("readdir , dir ino is %ld, contex->pos is %lld", dirInfo->thisIno, context->pos);
    DirInodeInfoIterateDentry(dirInfo, context->pos, NvmfsIterateFunc, context, &fsInfo->acc);
    return 0;
}

struct file_operations NvmfsDirFileOps = {.open = NvmfsFileOpen, .release = NvmfsFileRelease, .iterate = NvmfsIterate};
struct file_operations NvmfsRegFileOps = {
    .open = NvmfsFileOpen, .release = NvmfsFileRelease, .read = NvmfsFileRead, .write = NvmfsFileWrite};

// struct file_operations
// {
//     ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
//     ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
//     int (*open)(struct inode *, struct file *);
//     int (*release)(struct inode *, struct file *);
//     int (*iterate)(struct file *, struct dir_context *)
// }