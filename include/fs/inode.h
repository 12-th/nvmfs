#ifndef NVMFS_INODE_H
#define NVMFS_INODE_H

#include "Types.h"
#include <linux/time.h>

struct FsConstructor;
struct CircularBuffer;
struct NvmfsInfo;
struct InodeTable;
struct BlockPool;
struct NVMAccesser;
struct PagePool;

struct BaseInodeInfo
{
    UINT8 type;
    UINT32 linkCount;
    nvmfs_ino_t thisIno;
    nvmfs_ino_t parentIno;
    mode_t mode;
    struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;
};

int RootInodeFormat(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
int InodeFormat(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
struct BaseInodeInfo * InodeInfoAlloc(UINT8 type);
void BaseInodeInfoFormat(struct BaseInodeInfo * baseInfo, nvmfs_ino_t parentIno, mode_t mode, struct timespec curTime);
void InodeInfoFree(struct BaseInodeInfo * info);
void InodeUninit(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
void InodeRecovery(logic_addr_t inode, struct FsConstructor * ctor, struct CircularBuffer * cb,
                   struct NVMAccesser * acc);
void InodeDestroy(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
int InodeRebuild(struct BaseInodeInfo ** infoPtr, struct InodeTable * pTable, nvmfs_ino_t ino, struct PagePool * ppool,
                 struct BlockPool * bpool, struct NVMAccesser * acc);

void BaseInodeInfoPrint(struct BaseInodeInfo * info);

#endif