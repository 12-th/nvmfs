#ifndef FS_CONSTRUCTOR_H
#define FS_CONSTRUCTOR_H

#include "BlockPool.h"
#include "InodeTable.h"
#include "NVMAccesser.h"
#include "PagePool.h"
#include "Types.h"
#include "WearLeveler.h"

struct FsConstructor
{
    struct BlockPool * bpool;
    struct PagePool * ppool;
    struct WearLeveler * wl;
    struct NVMAccesser acc;
    struct InodeTable * inodeTable;
    nvm_addr_t inodeTableAddr;
    UINT64 inodeTableSize;
    UINT64 totalBlockNum;
};

void FsConstructInit(struct FsConstructor * ctor, struct BlockPool * bpool, struct PagePool * ppool,
                     struct WearLeveler * wl, struct InodeTable * inodeTable, nvm_addr_t inodeTableAddr,
                     UINT64 inodeTableSize, UINT64 totalBlockNum);
void FsConstructorBegin(struct FsConstructor * ctor);
void FsConstructorEnd(struct FsConstructor * ctor);
void FsConstructorRun(struct FsConstructor * ctor, nvmfs_ino_t rootIno);
void FsConstructorNotifyPageBusy(struct FsConstructor * ctor, logic_addr_t page);
void FsConstructorNotifyBlockBusy(struct FsConstructor * ctor, logic_addr_t startBlock, UINT64 blockNum);
void FsConstructorNotifyInodeInuse(struct FsConstructor * ctor, nvmfs_ino_t ino, int * hasNotified);

#endif