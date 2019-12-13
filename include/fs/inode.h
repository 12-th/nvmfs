#ifndef NVMFS_INODE_H
#define NVMFS_INODE_H

#include "Types.h"

struct FsConstructor;
struct CircularBuffer;
struct NvmfsInfo;
struct InodeTable;
struct BlockPool;
struct NVMAccesser;
struct PagePool;

#define INODE_TYPE_REGULAR_FILE 1
#define INODE_TYPE_DIR_FILE 2
#define INODE_TYPE_LINK_FILE 3

struct BaseInodeInfo
{
    UINT8 type;
    // struct timespec atime;
    // struct timespec mtime;
    // struct timespec ctime;
    UINT32 linkCount;
    nvmfs_ino_t thisIno;
    nvmfs_ino_t parentIno;
    mode_t mode;
};

int RootInodeFormat(struct NvmfsInfo * fsInfo, mode_t mode);
int InodeFormat(struct BaseInodeInfo ** infoPtr, struct NvmfsInfo * fsInfo, UINT8 type, nvmfs_ino_t parentIno,
                mode_t mode);
void InodeUninit(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
void InodeRecovery(logic_addr_t inode, struct FsConstructor * ctor, struct CircularBuffer * cb,
                   struct NVMAccesser * acc);
void InodeDestroy(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo);
int InodeRebuild(struct BaseInodeInfo ** infoPtr, struct InodeTable * pTable, nvmfs_ino_t ino, struct PagePool * ppool,
                 struct BlockPool * bpool, struct NVMAccesser * acc);

#endif