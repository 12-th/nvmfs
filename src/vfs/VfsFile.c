#include "DebugIoctl.h"
#include "DirInodeInfo.h"
#include "File.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "InodeType.h"
#include "common.h"
#include <linux/fs.h>

static int NvmfsFileOpen(struct inode * inode, struct file * file)
{
    if ((file->f_mode & FMODE_WRITE) && (file->f_flags & O_TRUNC))
        return InodeTruncate(inode->i_private, inode->i_sb->s_fs_info);
    return 0;
}

static int NvmfsFileRelease(struct inode * inode, struct file * file)
{
    // DEBUG_PRINT("file close, ino is %ld", ((struct BaseInodeInfo *)(inode->i_private))->thisIno);
    // DEBUG_PRINT("file close");
    return 0;
}

static ssize_t NvmfsFileRead(struct file * file, char __user * buffer, size_t size, loff_t * offset)
{
    struct FileInodeInfo * inodeInfo;
    struct NvmfsInfo * info;
    struct inode * pInode;
    ssize_t readSize;

    // DEBUG_PRINT("file read, offset is %ld, size is %ld", (unsigned long)(*offset), size);
    pInode = file->f_inode;
    inodeInfo = pInode->i_private;
    info = pInode->i_sb->s_fs_info;
    readSize = FileInodeInfoReadData(inodeInfo, buffer, size, *offset, &info->acc);

    pInode->i_blocks = FileInodeInfoGetPageNum(inodeInfo);

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
    struct inode * pInode;

    // DEBUG_PRINT("file write, offset is %ld, size is %ld\n", (unsigned long)(*offset), size);
    pInode = file->f_inode;
    inodeInfo = pInode->i_private;
    info = pInode->i_sb->s_fs_info;

    writeSize = FileInodeInfoWriteData(inodeInfo, (void *)buffer, size, *offset, &info->acc);

    pInode->i_blocks = FileInodeInfoGetPageNum(inodeInfo);
    i_size_write(pInode, FileInodeInfoGetMaxLen(inodeInfo));

    if (writeSize < 0)
        return writeSize;
    (*offset) += writeSize;
    return writeSize;
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
    // DEBUG_PRINT("readdir , dir ino is %ld, contex->pos is %lld", dirInfo->baseInfo.thisIno, context->pos);
    DirInodeInfoIterateDentry(dirInfo, context->pos, NvmfsIterateFunc, context, &fsInfo->acc);
    return 0;
}

static loff_t NvmfsLlseek(struct file * file, loff_t offset, int whence)
{
    struct FileInodeInfo * inodeInfo;
    UINT64 fileMaxLen;
    loff_t ret = 0;

    inodeInfo = file->f_inode->i_private;
    fileMaxLen = FileInodeInfoGetMaxLen(inodeInfo);
    switch (whence)
    {
    case SEEK_SET:
        ret = offset >= fileMaxLen ? fileMaxLen : offset;
        break;
    case SEEK_CUR:
        ret = offset + file->f_pos;
        if (ret < 0)
            ret = 0;
        break;
    case SEEK_END:
        ret = fileMaxLen - offset;
        if (ret < 0)
            ret = 0;
        break;
    }
    // DEBUG_PRINT("llseek, offset is %ld, whence is %ld, ret is %ld", (unsigned long)(offset), (unsigned long)(whence),
    //             (unsigned long)ret);
    file->f_pos = ret;
    return ret;
}

struct file_operations NvmfsDirFileOps = {
    .open = NvmfsFileOpen, .release = NvmfsFileRelease, .iterate = NvmfsIterate, .unlocked_ioctl = NvmfsUnlockedIoctl};
struct file_operations NvmfsRegFileOps = {.open = NvmfsFileOpen,
                                          .release = NvmfsFileRelease,
                                          .read = NvmfsFileRead,
                                          .write = NvmfsFileWrite,
                                          .llseek = NvmfsLlseek,
                                          .unlocked_ioctl = NvmfsUnlockedIoctl};

// struct file_operations
// {
//     ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
//     ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
//     int (*open)(struct inode *, struct file *);
//     int (*release)(struct inode *, struct file *);
//     int (*iterate)(struct file *, struct dir_context *)
// }