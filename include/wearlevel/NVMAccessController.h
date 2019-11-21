#ifndef MAP_TABLE_H
#define MAP_TABLE_H

#include "Types.h"

struct BlockUnmapTable;
struct PageUnmapTable;

struct PageEntry
{
    union {
        UINT16 value;
        struct
        {
            UINT64 busy : 1;
            UINT64 lock : 1;
            UINT64 relativeIndex : 9;
        };
    };
};

struct PageEntryTable
{
    struct PageEntry entries[512];
};

struct BlockEntry
{
    union {
        struct
        {
            UINT64 fineGrain : 1;
            UINT64 busy : 1;
            UINT64 lock : 2;
            UINT64 count : 8;
            UINT64 nvmAddr : 52;
        };
        UINT64 value;
    };
    struct PageEntryTable * ptr;
};

struct NVMAccessController
{
    struct BlockEntry * entries;
    UINT64 blockNum;
    UINT64 dataStartOffset;
    struct BlockUnmapTable * blockUnmapTable;
    struct PageUnmapTable * pageUnmapTable;
};

void NVMAccessControllerRebuild(struct NVMAccessController * acl, struct BlockUnmapTable * pBlockUnmapTable,
                                struct PageUnmapTable * pPageUnmapTable, UINT64 blockNum, nvm_addr_t dataStartOffset);
void NVMAccessControllerUninit(struct NVMAccessController * acl);
void NVMAccessControllerSharedLock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerSharedUnlock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerUniqueLock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerUniqueUnlock(struct NVMAccessController * acl, logic_addr_t addr);

void NVMAccessControllerBlockUniqueLock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerBlockUniqueUnlock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerBlockSharedLock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerBlockSharedUnlock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerPageLock(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerPageUnlock(struct NVMAccessController * acl, logic_addr_t addr);
int NVMAccessControllerIsBlockSplited(struct NVMAccessController * acl, logic_addr_t addr);

void NVMAccessControllerSplit(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerMerge(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerTrim(struct NVMAccessController * acl, logic_addr_t addr, int * shouldModifyNVMData);
void NVMAccessControllerAssureBusyFlagSet(struct NVMAccessController * acl, logic_addr_t addr,
                                          int * shouldModifyNVMData);
nvm_addr_t NVMAccessControllerLogicAddrTranslate(struct NVMAccessController * acl, logic_addr_t addr);
void NVMAccessControllerBlockSwapMap(struct NVMAccessController * acl, logical_block_t block1, logical_block_t block2);
void NVMAccessControllerPageSwapMap(struct NVMAccessController * acl, logical_page_t page1, logical_page_t page2);

#endif