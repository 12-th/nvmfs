#ifndef DIR_INODE_H
#define DIR_INODE_H

struct PagePool;
struct NvmInode;

#include "DirFileDentryCache.h"
#include "Log.h"
#include "Types.h"

#define INODE_LOG_ADD_DENTRY 1
#define INODE_LOG_REMOVE_DENTRY 2

struct DirInodeLogAddDentryEntry
{
    nvmfs_ino_t ino;
    nvmfs_inode_type type;
    UINT32 nameLen;
    char name[0];
};

struct DirInodeLogRemoveDentryEntry
{
    nvmfs_ino_t ino;
};

struct DirInodeInfo
{
    struct PagePool * pool;
    struct DirFileDentryCache cache;
    struct Log log;
};

int DirInodeInfoFormat(struct DirInodeInfo * info, struct PagePool * ppool, struct NvmInode * inodeData,
                       struct NVMAccesser * acc);
void DirInodeInfoUninit(struct DirInodeInfo * info);
void DirInodeInfoDestroy(struct DirInodeInfo * info, struct NVMAccesser * acc);
int DirInodeInfoLookupDentry(struct DirInodeInfo * info, nvmfs_ino_t ino);
int DirInodeAddDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, char * name, UINT64 len, UINT8 type,
                      struct NVMAccesser * acc);
int DirInodeRemoveDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, struct NVMAccesser * acc);
void DirInodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                      struct NVMAccesser * acc);
void DirInodeRebuild(struct DirInodeInfo * info, logic_addr_t addr, struct PagePool * pool, struct NVMAccesser * acc);

#endif