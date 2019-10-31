#include "Align.h"
#include "BlockUnmapTable.h"
#include "Layouter.h"
#include "MapTable.h"
#include "PageUnmapTable.h"
#include <linux/slab.h>

#define PGD_VALUE_FLAGS 0x3UL
#define PUD_VALUE_FLAGS 0x3UL
#define PMD_VALUE_2M_FLAGS 0x83UL
#define PMD_VALUE_4K_FLAGS 0x3UL
#define PTE_VALUE_FLAGS 0x3UL

#define PGD_VALUE_MASK 0xfffffffff000UL
#define PUD_VALUE_MASK 0xfffffffff000UL
#define PMD_VALUE_MASK 0xfffffffff000UL
#define PTE_VALUE_MASK 0xfffffffff000UL

#define PMD_2M_BIT_FLAG 0x80UL

static inline UINT64 PageRelativeIndexToPte(UINT64 physRelativeIndex, physical_block_t block, UINT64 base)
{
    return (base + (block << BITS_2M) + (physRelativeIndex << BITS_4K)) | PTE_VALUE_FLAGS;
}

static inline UINT64 BlockNumberToPmd2MGrain(UINT64 block, UINT64 base)
{
    return (base + (block << BITS_2M)) | PMD_VALUE_2M_FLAGS;
}

static inline UINT64 PageNumberToPte(UINT64 page, UINT64 base)
{
    return (base + (page << BITS_4K)) | PTE_VALUE_FLAGS;
}

static inline UINT64 BlockNumberToPmd4KGrain(struct PtEntry * ptTable)
{
    return ((UINT64)__pa(ptTable)) | PMD_VALUE_4K_FLAGS;
}

static void ModifyPageTable(struct MapTable * pTable, UINT64 virt, UINT64 value, enum MapLevel level, UINT64 * oldValue)
{
    UINT64 indexOfPgd = (virt & (0x1ffUL << 38)) >> 38;
    UINT64 indexOfPud = (virt & (0x1ffUL << 29)) >> 29;
    UINT64 indexOfPmd = (virt & (0x1ffUL << 20)) >> 20;
    UINT64 indexOfPte = (virt & (0x1ffUL << 12)) >> 12;
    struct PgdEntry * pgd;
    struct PudEntry * pud;
    struct PmdEntry * pmd;
    struct PtEntry * pte;

    pgd = &pTable->entries[indexOfPgd];
    if (pgd->value == 0UL)
    {
        pgd->value = ((UINT64)__pa(__get_free_page(GFP_KERNEL | __GFP_ZERO))) | PGD_VALUE_FLAGS;
    }
    pud = (struct PudEntry *)__va((void *)(pgd->value & PGD_VALUE_MASK)) + indexOfPud;
    if (pud->value == 0UL)
    {
        pud->value = ((UINT64)__pa(__get_free_page(GFP_KERNEL | __GFP_ZERO))) | PUD_VALUE_FLAGS;
    }
    pmd = (struct PmdEntry *)__va((void *)(pud->value & PUD_VALUE_MASK)) + indexOfPmd;

    if (level == LEVEL_2M)
    {
        if (oldValue)
            *oldValue = pmd->value;
        pmd->value = value;
        return;
    }
    if (pmd->value == 0UL)
    {
        pmd->value = ((UINT64)__pa(__get_free_page(GFP_KERNEL | __GFP_ZERO))) | PMD_VALUE_4K_FLAGS;
    }
    pte = (struct PtEntry *)__va((void *)(pmd->value & PMD_VALUE_MASK)) + indexOfPte;
    if (oldValue)
        *oldValue = pte->value;
    pte->value = value;
}

void MapTableRebuild(struct MapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable,
                     struct PageUnmapTable * pPageUnmapTable, UINT64 blockNum, UINT64 nvmBaseAddr,
                     nvm_addr_t dataStartOffset)
{
    UINT64 i;
    UINT64 * buffer;
    UINT64 base;
    UINT64 entryNum;

    entryNum = AlignUp(blockNum, 1UL << (39 - BITS_2M)) >> (39 - BITS_2M);
    pTable->entries = kzmalloc(sizeof(struct PgdEntry) * entryNum, GFP_KERNEL);
    pTable->entryNum = entryNum;
    pTable->nvmBaseAddr = nvmBaseAddr;
    pTable->dataStartOffset = dataStartOffset;
    base = nvmBaseAddr + dataStartOffset;

    buffer = __get_free_page(GFP_KERNEL);
    for (i = 0; i < blockNum; ++i)
    {
        struct BlockInfo info;
        UINT64 virt;

        BlockUnmapTableGet(pBlockUnmapTable, i, &info);
        virt = info.unmapBlock << BITS_2M;
        if (info.fineGrain)
        {
            UINT64 * pt;
            UINT64 j;

            pt = __get_free_page(GFP_KERNEL);
            PageUnmapTableBatchGet(pPageUnmapTable, i << (BITS_2M - BITS_4K), buffer);
            for (j = 0; j < 512; ++j)
            {
                pt[buffer[j]] = PageRelativeIndexToPte(j, i, base);
            }
            ModifyPageTable(pTable, virt, BlockNumberToPmd4KGrain((struct PtEntry *)pt), LEVEL_4K, NULL);
        }
        else
        {
            ModifyPageTable(pTable, virt, BlockNumberToPmd2MGrain(i, base), LEVEL_2M, NULL);
        }
    }

    free_page((unsigned long)buffer);
}

static inline void FreePmdTable(struct PmdEntry * pmd)
{
    struct PtEntry * pmdTable;

    if (!(pmd->value & PMD_2M_BIT_FLAG))
    {
        pmdTable = __va((void *)(pmd->value & PMD_VALUE_MASK));
        free_page((unsigned long)pmdTable);
    }
}

static inline void FreePudTable(struct PudEntry * pud)
{
    int i;
    struct PmdEntry * pudTable;

    pudTable = __va((void *)(pud->value & PUD_VALUE_MASK));
    for (i = 0; i < 512; ++i)
    {
        struct PmdEntry * pmd;
        pmd = &pudTable[i];
        if (pmd->value)
        {
            FreePmdTable(pmd);
        }
    }
    free_page((unsigned long)pudTable);
}

static inline void FreePgdTable(struct PgdEntry * pgd)
{
    int i;
    struct PudEntry * pgdTable;

    pgdTable = __va((void *)(pgd->value & PGD_VALUE_MASK));
    for (i = 0; i < 512; ++i)
    {
        struct PudEntry * pud;
        pud = &pgdTable[i];
        if (pud->value)
        {
            FreePudTable(pud);
        }
    }
    free_page((unsigned long)pgdTable);
}

void MapTableUninit(struct MapTable * pTable)
{
    int i;
    struct PgdEntry * pgd;
    for (i = 0; i < pTable->entryNum; ++i)
    {
        pgd = &pTable->entries[i];
        if (pgd->value)
        {
            FreePgdTable(pgd);
        }
    }
    kfree(pTable->entries);
}

nvm_addr_t MapAddressQuery(struct MapTable * pTable, UINT64 virt)
{
    UINT64 indexOfPgd = (virt & (0x1ffUL << 38)) >> 38;
    UINT64 indexOfPud = (virt & (0x1ffUL << 29)) >> 29;
    UINT64 indexOfPmd = (virt & (0x1ffUL << 20)) >> 20;
    UINT64 indexOfPte = (virt & (0x1ffUL << 12)) >> 12;
    struct PgdEntry * pgd;
    struct PudEntry * pud;
    struct PmdEntry * pmd;
    struct PtEntry * pte;

    pgd = &pTable->entries[indexOfPgd];
    if (pgd->value == 0)
        return invalid_nvm_addr;

    pud = (struct PudEntry *)__va((void *)(pgd->value & PGD_VALUE_MASK)) + indexOfPud;
    if (pud->value == 0UL)
        return invalid_nvm_addr;

    pmd = (struct PmdEntry *)__va((void *)(pud->value & PUD_VALUE_MASK)) + indexOfPmd;
    if (pmd->value == 0UL)
        return invalid_nvm_addr;
    if (pmd->value & PMD_2M_BIT_FLAG)
        return (pmd->value & PMD_VALUE_MASK) - pTable->nvmBaseAddr;
    pte = (struct PtEntry *)__va((void *)(pmd->value & PMD_VALUE_MASK)) + indexOfPte;
    if (pte->value == 0UL)
        return invalid_nvm_addr;
    return (pte->value & PTE_VALUE_MASK) - pTable->nvmBaseAddr;
}

void BlockMapModify(struct MapTable * pTable, logical_block_t virtBlock, physical_block_t block)
{
    UINT64 value;

    value = BlockNumberToPmd2MGrain(block, pTable->nvmBaseAddr + pTable->dataStartOffset);
    ModifyPageTable(pTable, virtBlock << BITS_2M, value, LEVEL_2M, NULL);
}

void PageMapModify(struct MapTable * pTable, logical_page_t virtPage, physical_page_t page)
{
    UINT64 value;

    value = PageNumberToPte(page, pTable->nvmBaseAddr + pTable->dataStartOffset);
    ModifyPageTable(pTable, virtPage << BITS_4K, value, LEVEL_4K, NULL);
}

void MapTableSplitBlockMap(struct MapTable * pTable, logical_block_t virtBlock, physical_block_t block)
{
    struct PtEntry * ptTable;
    int i;
    UINT64 base;
    UINT64 pmdValue;

    base = pTable->nvmBaseAddr + pTable->dataStartOffset;
    ptTable = __get_free_page(GFP_KERNEL);
    for (i = 0; i < 512; ++i)
    {
        ptTable[i].value = PageRelativeIndexToPte(i, block, base);
    }
    pmdValue = BlockNumberToPmd4KGrain(ptTable);
    ModifyPageTable(pTable, virtBlock << BITS_2M, pmdValue, LEVEL_2M, NULL);
}