#ifndef DIR_INODE_H
#define DIR_INODE_H

struct PagePool;
struct NvmInode;

#include "DirFileDentryCache.h"
#include "Inode.h"
#include "Log.h"
#include "Types.h"

#define INODE_LOG_ADD_DENTRY 1
#define INODE_LOG_REMOVE_DENTRY 2

struct DirInodeLogAddDentryEntry
{
    nvmfs_ino_t ino;
    nvmfs_inode_type type;
    UINT32 nameLen;
    UINT32 nameHash;
    char name[0];
};

struct DirInodeLogRemoveDentryEntry
{
    nvmfs_ino_t ino;
};

struct DirInodeInfo
{
    struct BaseInodeInfo baseInfo;
    struct PagePool * pool;
    struct DirFileDentryCache cache;
    struct Log log;
    nvmfs_ino_t thisIno;
    nvmfs_ino_t parentIno;
};

int DirInodeInfoFormat(struct DirInodeInfo * info, struct PagePool * ppool, logic_addr_t * firstArea,
                       struct NVMAccesser * acc);
void DirInodeInfoUninit(struct DirInodeInfo * info);
void DirInodeInfoDestroy(struct DirInodeInfo * info, struct NVMAccesser * acc);
int DirInodeInfoLookupDentryByIno(struct DirInodeInfo * info, nvmfs_ino_t ino);
int DirInodeInfoLookupAndGetDentryName(struct DirInodeInfo * info, nvmfs_ino_t ino, char * buffer, UINT64 bufferSize,
                                       struct NVMAccesser * acc);
int DirInodeInfoLookupDentryByName(struct DirInodeInfo * info, nvmfs_ino_t * retIno, char * name, UINT64 nameLen,
                                   struct NVMAccesser * acc);
int DirInodeInfoAddDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, char * name, UINT64 len, UINT8 type,
                          struct NVMAccesser * acc);
int DirInodeInfoRemoveDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, struct NVMAccesser * acc);
void DirInodeInfoIterateDentry(struct DirInodeInfo * info, UINT64 index,
                               int (*func)(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino),
                               void * data, struct NVMAccesser * acc);
void DirInodeInfoRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                          struct NVMAccesser * acc);
void DirInodeInfoRebuild(struct DirInodeInfo * info, logic_addr_t addr, struct PagePool * pool,
                         struct NVMAccesser * acc);

#endif