#include "Align.h"
#include "BlockUnmapTable.h"
#include "Layouter.h"
#include "NVMAccessController.h"
#include "NVMOperations.h"
#include "PageUnmapTable.h"
#include <linux/mm.h>
#include <linux/slab.h>

static inline UINT64 IndexOfBlock(logic_addr_t addr)
{
    return (addr & (0x1ffUL << 21)) >> 21;
}

static inline UINT64 IndexOfPage(logic_addr_t addr)
{
    return (addr & (0x1ffUL << 12)) >> 12;
}

static inline UINT64 OffsetOfPage(logic_addr_t addr)
{
    return (addr & ((1UL << 12) - 1));
}

static inline UINT64 OffsetOfBlock(logic_addr_t addr)
{
    return (addr & ((1UL << 21) - 1));
}

static inline UINT64 BlockEntryPackNVMAddr(nvm_addr_t addr)
{
    return addr >> 21;
}

static inline nvm_addr_t BlockEntryUnpackNVMAddr(UINT64 addr)
{
    return addr << 21;
}

static inline struct PageEntry BuildPageEntry(UINT64 relativeIndex, UINT64 busy)
{
    struct PageEntry entry = {.relativeIndex = relativeIndex, .busy = busy, .lock = 0};
    return entry;
}

static inline struct BlockEntry BuildBlockEntry(UINT64 block, UINT64 base, UINT64 busy, struct PageEntryTable * ptr,
                                                UINT64 fineGrain)
{
    struct BlockEntry entry = {.fineGrain = fineGrain,
                               .busy = busy,
                               .lock = 0,
                               .count = 0,
                               .nvmAddr = BlockEntryPackNVMAddr(block_to_nvm_addr(block, base)),
                               .ptr = ptr};
    return entry;
}

static inline void BlockMapSet(struct NVMAccessController * acl, logic_addr_t addr, struct BlockEntry entry)
{
    acl->entries[IndexOfBlock(addr)] = entry;
}

static inline struct BlockEntry BlockMapGet(struct NVMAccessController * acl, logic_addr_t addr)
{
    return acl->entries[IndexOfBlock(addr)];
}

static inline void PageMapSet(struct NVMAccessController * acl, logic_addr_t addr, struct PageEntry entry)
{
    struct BlockEntry * be;
    struct PageEntry * pe;

    be = &acl->entries[IndexOfBlock(addr)];
    ASSERT(be->fineGrain == 1);
    pe = be->ptr->entries + IndexOfPage(addr);
    *pe = entry;
}

static inline struct PageEntry PageMapGet(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;
    struct PageEntry * pe;

    be = &acl->entries[IndexOfBlock(addr)];
    ASSERT(be->fineGrain == 1);
    pe = be->ptr->entries + IndexOfPage(addr);
    return *pe;
}

void NVMAccessControllerRebuild(struct NVMAccessController * acl, struct BlockUnmapTable * pBlockUnmapTable,
                                struct PageUnmapTable * pPageUnmapTable, UINT64 blockNum, nvm_addr_t dataStartOffset)
{
    UINT64 i;
    struct PageInfo * pageInfos;

    acl->entries = kvmalloc(sizeof(struct BlockEntry) * blockNum, GFP_KERNEL | __GFP_ZERO);
    acl->blockNum = blockNum;
    acl->dataStartOffset = dataStartOffset;
    acl->blockUnmapTable = pBlockUnmapTable;
    acl->pageUnmapTable = pPageUnmapTable;

    pageInfos = kmalloc(sizeof(struct PageInfo) * 512, GFP_KERNEL);
    for (i = 0; i < blockNum; ++i)
    {
        struct BlockInfo blockInfo;
        UINT64 virt;

        BlockUnmapTableGet(pBlockUnmapTable, i, &blockInfo);
        virt = blockInfo.unmapBlock << BITS_2M;
        if (blockInfo.fineGrain)
        {
            struct PageEntryTable * pet;
            UINT64 j;

            pet = kmalloc(sizeof(struct PageEntryTable), GFP_KERNEL);
            PageUnmapTableBatchGet(pPageUnmapTable, i, pageInfos);
            for (j = 0; j < 512; ++j)
            {
                pet->entries[pageInfos[j].unmapPage] = BuildPageEntry(j, pageInfos[j].busy);
            }
            BlockMapSet(acl, virt, BuildBlockEntry(i, dataStartOffset, blockInfo.busy, pet, 1));
        }
        else
        {
            BlockMapSet(acl, virt, BuildBlockEntry(i, dataStartOffset, blockInfo.busy, NULL, 0));
        }
    }

    kfree(pageInfos);
}

void NVMAccessControllerUninit(struct NVMAccessController * acl)
{
    int i;
    for (i = 0; i < acl->blockNum; ++i)
    {
        if (acl->entries[i].fineGrain)
        {
            kfree(acl->entries[i].ptr);
        }
    }
    kvfree(acl->entries);
}

static inline void BlockEntrySharedLock(struct BlockEntry * ptr)
{
    struct BlockEntry oldEntry;
    struct BlockEntry newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        if (oldEntry.lock == 0)
        {
            newEntry.lock = 2;
            newEntry.count = 1;
        }
        else
        {
            oldEntry.lock = 2;
            newEntry.lock = 2;
            newEntry.count++;
        }
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
}

static inline void BlockEntrySharedUnlock(struct BlockEntry * ptr)
{
    struct BlockEntry oldEntry;
    struct BlockEntry newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        if (oldEntry.lock == 2)
        {
            if (oldEntry.count == 1)
            {
                newEntry.lock = 0;
                newEntry.count = 0;
            }
            else
            {
                newEntry.count--;
            }
        }
        else
        {
            oldEntry.lock = 3;
            if (oldEntry.count == 1)
            {
                newEntry.lock = 1;
                newEntry.count = 0;
            }
            else
            {
                newEntry.count--;
            }
        }
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
}

static inline void BlockEntryUniqueLock(struct BlockEntry * ptr)
{
    struct BlockEntry oldEntry;
    struct BlockEntry newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        if (oldEntry.lock == 0)
        {
            newEntry.lock = 1;
        }
        else
        {
            oldEntry.lock = 2;
            newEntry.lock = 3;
        }
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
    do
    {
        oldEntry = *ptr;
    } while (oldEntry.lock != 1);
}

static inline void BlockEntryUniqueUnlock(struct BlockEntry * ptr)
{
    struct BlockEntry oldEntry;
    struct BlockEntry newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        newEntry.lock = 0;
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
}

static inline void PageEntryLock(struct PageEntry * ptr)
{
    struct PageEntry oldEntry, newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        newEntry.lock = 1;
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
}

static inline void PageEntryUnlock(struct PageEntry * ptr)
{
    struct PageEntry oldEntry, newEntry;
    do
    {
        oldEntry = *ptr;
        newEntry = oldEntry;
        newEntry.lock = 0;
    } while (!BOOL_COMPARE_AND_SWAP(&ptr->value, oldEntry.value, newEntry.value));
}

static inline void SharedLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    BlockEntrySharedLock(be);
    if (be->fineGrain)
    {
        PageEntryLock(be->ptr->entries + IndexOfPage(addr));
    }
}

static inline void SharedUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    if (be->fineGrain)
    {
        PageEntryUnlock(be->ptr->entries + IndexOfPage(addr));
    }
    BlockEntrySharedUnlock(be);
}

static inline void UniqueLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    BlockEntryUniqueLock(be);
    if (be->fineGrain)
    {
        PageEntryLock(be->ptr->entries + IndexOfPage(addr));
    }
}

static inline void UniqueUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    if (be->fineGrain)
    {
        PageEntryUnlock(be->ptr->entries + IndexOfPage(addr));
    }
    BlockEntryUniqueUnlock(be);
}

static inline nvm_addr_t LogicAddressTranslate(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;
    nvm_addr_t physAddr;

    be = &acl->entries[IndexOfBlock(addr)];
    physAddr = BlockEntryUnpackNVMAddr(be->nvmAddr);
    if (be->fineGrain)
    {
        struct PageEntry * pe;

        pe = be->ptr->entries + IndexOfPage(addr);
        physAddr += (pe->relativeIndex << BITS_4K);
        return physAddr + OffsetOfPage(addr);
    }
    return physAddr + OffsetOfBlock(addr);
}

void NVMAccessControllerSharedLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    SharedLock(acl, addr);
}

void NVMAccessControllerSharedUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    SharedUnlock(acl, addr);
}

void NVMAccessControllerUniqueLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    UniqueLock(acl, addr);
}

void NVMAccessControllerUniqueUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    UniqueUnlock(acl, addr);
}

void NVMAccessControllerBlockUniqueLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    BlockEntryUniqueLock(acl->entries + IndexOfBlock(addr));
}

void NVMAccessControllerBlockUniqueUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    BlockEntryUniqueUnlock(acl->entries + IndexOfBlock(addr));
}

void NVMAccessControllerBlockSharedLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    BlockEntrySharedLock(acl->entries + IndexOfBlock(addr));
}

void NVMAccessControllerBlockSharedUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    BlockEntrySharedUnlock(acl->entries + IndexOfBlock(addr));
}

void NVMAccessControllerPageLock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    ASSERT(be->fineGrain);
    PageEntryLock(be->ptr->entries + IndexOfPage(addr));
}

void NVMAccessControllerPageUnlock(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;
    be = &acl->entries[IndexOfBlock(addr)];
    ASSERT(be->fineGrain);
    PageEntryUnlock(be->ptr->entries + IndexOfPage(addr));
}

int NVMAccessControllerIsBlockSplited(struct NVMAccessController * acl, logic_addr_t addr)
{
    return acl->entries[IndexOfBlock(addr)].fineGrain;
}

void NVMAccessControllerAssureBusyFlagSet(struct NVMAccessController * acl, logic_addr_t addr,
                                          int * shouldModifyNVMData)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    *shouldModifyNVMData = 0;
    if (!be->busy)
    {
        be->busy = 1;
        *shouldModifyNVMData = 1;
    }
    if (be->fineGrain)
    {
        struct PageEntry * pe;

        pe = be->ptr->entries + IndexOfPage(addr);
        if (!pe->busy)
        {
            pe->busy = 1;
            *shouldModifyNVMData = 1;
        }
    }
}

void NVMAccessControllerSplit(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    if (!be->fineGrain)
    {
        struct PageEntryTable * pet;
        UINT64 i;

        pet = kmalloc(sizeof(struct PageEntryTable), GFP_KERNEL);
        for (i = 0; i < 512; ++i)
        {
            pet->entries[i] = BuildPageEntry(i, be->busy);
        }
        be->ptr = pet;
        be->fineGrain = 1;
    }
}

void NVMAccessControllerMerge(struct NVMAccessController * acl, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    if (be->fineGrain)
    {
        be->fineGrain = 1;
        kfree(be->ptr);
    }
}

void NVMAccessControllerTrim(struct NVMAccessController * acl, logic_addr_t addr, int * shouldModifyNVMData)
{
    struct BlockEntry * be;

    be = &acl->entries[IndexOfBlock(addr)];
    *shouldModifyNVMData = 0;
    if (be->fineGrain)
    {
        struct PageEntry * pe;

        pe = be->ptr->entries + IndexOfPage(addr);
        if (pe->busy)
        {
            pe->busy = 0;
            *shouldModifyNVMData = 1;
        }
    }
    else
    {
        if (be->busy)
        {
            be->busy = 0;
            *shouldModifyNVMData = 1;
        }
    }
}

nvm_addr_t NVMAccessControllerLogicAddrTranslate(struct NVMAccessController * acl, logic_addr_t addr)
{
    return LogicAddressTranslate(acl, addr);
}

void NVMAccessControllerSwapMap(struct NVMAccessController * acl, logic_addr_t addr1, logic_addr_t addr2)
{
    struct BlockEntry *be1, *be2;
    be1 = &acl->entries[IndexOfBlock(addr1)];
    be2 = &acl->entries[IndexOfBlock(addr2)];

    ASSERT(be1->fineGrain == be2->fineGrain);
    if (!be1->fineGrain)
    {
        struct BlockEntry tmp;
        tmp = *be1;
        *be1 = *be2;
        *be2 = tmp;
    }
    else
    {
        struct PageEntry *pe1, *pe2;
        struct PageEntry tmp;
        pe1 = be1->ptr->entries + IndexOfPage(addr1);
        pe2 = be2->ptr->entries + IndexOfPage(addr2);
        tmp = *pe1;
        *pe1 = *pe2;
        *pe2 = tmp;
    }
}

void NVMAccessControllerBlockSwapMap(struct NVMAccessController * acl, logical_block_t block1, logical_block_t block2)
{
    struct BlockEntry *be1, *be2;
    UINT64 nvmAddr;
    be1 = &acl->entries[block1];
    be2 = &acl->entries[block2];
    nvmAddr = be1->nvmAddr;
    be1->nvmAddr = be2->nvmAddr;
    be2->nvmAddr = nvmAddr;
}

void NVMAccessControllerPageSwapMap(struct NVMAccessController * acl, logical_page_t page1, logical_page_t page2)
{
    struct BlockEntry *be1, *be2;
    struct PageEntry *pe1, *pe2;
    UINT64 tmp;
    be1 = &acl->entries[logical_page_to_block(page1)];
    be2 = &acl->entries[logical_page_to_block(page2)];
    pe1 = be1->ptr->entries + page1;
    pe2 = be2->ptr->entries + page2;
    tmp = pe1->relativeIndex;
    pe1->relativeIndex = pe2->relativeIndex;
    pe2->relativeIndex = tmp;
}