#include "CircularBuffer.h"
#include "FsConstructor.h"
#include "Inode.h"

void FsConstructInit(struct FsConstructor * ctor, struct BlockPool * bpool, struct PagePool * ppool,
                     struct WearLeveler * wl, struct InodeTable * inodeTable, nvm_addr_t inodeTableAddr,
                     UINT64 inodeTableSize, UINT64 totalBlockNum)
{
    ctor->bpool = bpool;
    ctor->ppool = ppool;
    ctor->wl = wl;
    ctor->inodeTable = inodeTable;
    ctor->inodeTableAddr = inodeTableAddr;
    ctor->inodeTableSize = inodeTableSize;
    ctor->totalBlockNum = totalBlockNum;
}

void FsConstructorBegin(struct FsConstructor * ctor)
{
    WearLevelerInit(ctor->wl);
    NVMAccesserInit(&ctor->acc, ctor->wl);
    BlockPoolRecoveryInit(ctor->bpool);
    PagePoolRecoveryInit(ctor->ppool, ctor->bpool, ctor->acc);
    InodeTableRecoveryPreinit(ctor->inodeTable, ctor->inodeTableAddr, ctor->inodeTableSize);
}

void FsConstructorEnd(struct FsConstructor * ctor)
{
    BlockPoolRecoveryEnd(ctor->bpool, ctor->totalBlockNum);
    PagePoolRecoveryEnd(ctor->ppool);
    InodeTableRecoveryEnd(ctor->inodeTable);
}

void FsConstructorNotifyPageBusy(struct FsConstructor * ctor, logic_addr_t page)
{
    PagePoolRecoveryNotifyPageBusy(ctor->ppool, page);
}

void FsConstructorNotifyBlockBusy(struct FsConstructor * ctor, logic_addr_t startAddr, UINT64 blockNum)
{
    BlockPoolRecoveryNotifyBlockBusy(ctor->bpool, logical_addr_to_block(startAddr), blockNum);
}

void FsConstructorNotifyInodeInuse(struct FsConstructor * ctor, nvmfs_ino_t ino, int * hasNotified)
{
    InodeTableRecoveryNotifyInodeInuse(ctor->inodeTable, ino, hasNotified);
}

void FsConstructorRun(struct FsConstructor * ctor, nvmfs_ino_t rootIno)
{
    struct CircularBuffer buffer;

    CircularBufferInit(&buffer);
    CircularBufferAdd(&buffer, (void *)rootIno);

    while (!CircularBufferIsEmpty(&buffer))
    {
        nvmfs_ino_t ino;
        int hasNotified;
        ino = (nvmfs_ino_t)CircularBufferDelete(&buffer);
        InodeTableRecoveryNotifyInodeInuse(ctor->inodeTable, ino, &hasNotified);
        if (!hasNotified)
        {
            logic_addr_t inode = InodeTableGetInodeAddr(ctor->inodeTable, ino);
            InodeRecovery(inode, ctor, &buffer, &ctor->acc);
        }
    }

    CircularBufferUninit(&buffer);
}