#ifndef NVMFS_FILE_INODE_H
#define NVMFS_FILE_INODE_H

#include "FileDataManager.h"
#include "Inode.h"
#include "Log.h"

#define INODE_LOG_ADD_SPACE 1
#define INODE_LOG_WRITE_DATA 2

struct InodeLogWriteDataEntry
{
    UINT64 fileStart;
    logic_addr_t addr;
    UINT64 size;
};

struct InodeLogAddNewSpaceEntry
{
    logic_addr_t addr;
    UINT64 size;
};

struct FileInodeInfo
{
    logic_addr_t addr;
    struct Log log;
    struct FileDataManager manager;
};

int FileInodeInfoFormat(struct FileInodeInfo * info, struct PagePool * ppool, struct BlockPool * bpool,
                        struct NvmInode * inodeData, struct NVMAccesser * acc);
void FileInodeInfoUninit(struct FileInodeInfo * info);
void FileInodeInfoDestroy(struct FileInodeInfo * info, struct NVMAccesser * acc);
void FileInodeInfoReadData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                           struct NVMAccesser * acc);
void FileInodeInfoWriteData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                            struct NVMAccesser * acc);
void FileInodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                       struct NVMAccesser * acc);
void FileInodeRebuild(struct FileInodeInfo * info, logic_addr_t inodeAddr, struct PagePool * ppool,
                      struct BlockPool * bpool, struct NVMAccesser * acc);

#endif