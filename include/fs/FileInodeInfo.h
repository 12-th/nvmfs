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
    struct BaseInodeInfo baseInfo;
    struct Log log;
    struct FileDataManager manager;
};

int FileInodeInfoFormat(struct FileInodeInfo * info, struct PagePool * ppool, struct BlockPool * bpool,
                        logic_addr_t * firstArea, struct NVMAccesser * acc);
void FileInodeInfoUninit(struct FileInodeInfo * info);
void FileInodeInfoDestroy(struct FileInodeInfo * info, struct NVMAccesser * acc);
int FileInodeInfoReadData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                          struct NVMAccesser * acc);
INT64 FileInodeInfoWriteData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                             struct NVMAccesser * acc);
void FileInodeInfoRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                           struct NVMAccesser * acc);
void FileInodeInfoRebuild(struct FileInodeInfo * info, logic_addr_t inodeAddr, struct PagePool * ppool,
                          struct BlockPool * bpool, struct NVMAccesser * acc);
static inline UINT64 FileInodeInfoGetMaxLen(struct FileInodeInfo * info)
{
    return info->manager.fileMaxLen;
}

static inline UINT64 FileInodeInfoGetPageNum(struct FileInodeInfo * info)
{
    return FileDataManagerGetSpaceSize(&info->manager) / SIZE_4K + LogTotalSizeQuery(&info->log) / SIZE_4K;
}

int FileInodeIsInfoSame(struct FileInodeInfo * info1, struct FileInodeInfo * info2);
void FileInodePrintInfo(struct FileInodeInfo * info);

#endif