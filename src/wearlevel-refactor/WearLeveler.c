#include "WearLeveler.h"

void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize)
{
    struct Layouter * l = &wl->layouter;
    UINT64 blockNum, pageNum;

    LayouterInit(l, nvmSizeBits, reserveSize);
    blockNum = BlockNumQuery(l);
    pageNum = PageNumQuery(l);
    MapTableFormat(&wl->mapTable, blockNum, l->dataStart);

    wl->blockColdnessManager = BlockColdnessManagerFormat(blockNum, LOGIC_BLOCK_RESERVE_NUM);
    BlockSwapLogFormat(&wl->blockSwapLog, l->metaDataOfBlockSwapLog,
                       MapTableQuery(&wl->mapTable, l->blockSwapLogLogicAddr));
    BlockUnmapTableFormat(&wl->blockUnmapTable, l->blockUnmapTable, blockNum);
    BlockWearTableFormat(&wl->blockWearTable, l->blockWearTable, blockNum);

    PageColdnessManagerFormat(&wl->pageColdnessManager, pageNum);
    PageSwapAreaFormat(&wl->pageSwapArea, MapTableQuery(&wl->mapTable, l->pageSwapAreaLogicAddr),
                       l->metaDataOfPageSwapArea);
    PageSwapLogFormat(&wl->pageSwapLog, l->metaDataOfPageSwapLog,
                      MapTableQuery(&wl->mapTable, l->pageSwapLogLogicAddr));
    PageUnmapTableFormat(&wl->pageUnmapTable, &wl->blockUnmapTable, l->pageUnmapTable);
    PageWearTableFormat(&wl->pageWearTable, l->pageWearTable, pageNum);

    BlockManagerInit(&wl->blockManager, wl);
    PageManagerInit(&wl->pageManager, wl);

    BlockColdnessManagerRemove(wl->blockColdnessManager,
                               nvm_addr_to_block(MapTableQuery(&wl->mapTable, l->blockSwapLogLogicAddr), l->dataStart));
    BlockColdnessManagerRemove(wl->blockColdnessManager,
                               nvm_addr_to_block(MapTableQuery(&wl->mapTable, l->pageSwapLogLogicAddr), l->dataStart));
    BlockColdnessManagerRemove(wl->blockColdnessManager,
                               nvm_addr_to_block(MapTableQuery(&wl->mapTable, l->pageSwapAreaLogicAddr), l->dataStart));
}

void WearLevelerInit(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize)
{
}

void WearLevelerUninit(struct WearLeveler * wl)
{
    PageWearTableUninit(&wl->pageWearTable);
    PageUnmapTableUninit(&wl->pageUnmapTable);
    PageSwapLogUninit(&wl->pageSwapLog);
    PageSwapAreaUninit(&wl->pageSwapArea);
    PageColdnessManagerUninit(&wl->pageColdnessManager);
    BlockWearTableUninit(&wl->blockWearTable);
    BlockUnmapTableUninit(&wl->blockUnmapTable);
    BlockSwapLogUninit(&wl->blockSwapLog);
    BlockColdnessManagerUninit(wl->blockColdnessManager);
    MapTableUninit(&wl->mapTable);
}
void NVMBlockSplit(struct WearLeveler * wl, logic_addr_t addr)
{
    if (MapTableIsBlock(&wl->mapTable, addr))
        BlockManagerSplit(&wl->blockManager, addr);
}

void NVMPagesMerge(struct WearLeveler * wl, logic_addr_t addr)
{
    if (!MapTableIsBlock(&wl->mapTable, addr))
        BlockManagerMerge(&wl->blockManager, addr);
}

UINT32 WearLevelerRead(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size)
{
    UINT32 canReadSize = 0;
    if (MapTableIsBlock(&wl->mapTable, addr))
    {
        BlockManagerRead(&wl->blockManager, addr, size, buffer);
        canReadSize = SIZE_2M - (addr & ((1UL << BITS_2M) - 1));
    }
    else
    {
        PageManagerRead(&wl->pageManager, addr, size, buffer);
        canReadSize = SIZE_4K - (addr & ((1UL << BITS_4K) - 1));
    }

    return canReadSize > size ? size : canReadSize;
}

UINT32 WearLevelerWrite(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size,
                        UINT32 increasedWearCount)
{
    UINT32 canWriteSize = 0;
    if (MapTableIsBlock(&wl->mapTable, addr))
    {
        BlockManagerWrite(&wl->blockManager, addr, size, buffer, increasedWearCount);
        canWriteSize = SIZE_2M - (addr & ((1UL << BITS_2M) - 1));
    }
    else
    {
        PageManagerWrite(&wl->pageManager, addr, size, buffer, increasedWearCount);
        canWriteSize = SIZE_4K - (addr & ((1UL << BITS_4K) - 1));
    }

    return canWriteSize > size ? size : canWriteSize;
}

UINT32 WearLevelerMemset(struct WearLeveler * wl, logic_addr_t addr, UINT32 size, int value, UINT32 increasedWearCount)
{
    UINT32 canWriteSize = 0;
    if (MapTableIsBlock(&wl->mapTable, addr))
    {
        BlockManagerMemset(&wl->blockManager, addr, size, value, increasedWearCount);
        canWriteSize = SIZE_2M - (addr & ((1UL << BITS_2M) - 1));
    }
    else
    {
        PageManagerMemset(&wl->pageManager, addr, size, value, increasedWearCount);
        canWriteSize = SIZE_4K - (addr & ((1UL << BITS_4K) - 1));
    }

    return canWriteSize > size ? size : canWriteSize;
}

UINT32 WearLevelerMemcpy(struct WearLeveler * wl, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT32 size,
                         UINT32 increasedWearCount, void * tmpBuffer)
{
    UINT32 srcCanReadSize, dstCanWriteSize, copySize;
    if (MapTableIsBlock(&wl->mapTable, srcAddr))
        srcCanReadSize = SIZE_2M - (srcAddr & ((1UL << BITS_2M) - 1));
    else
        srcCanReadSize = SIZE_4K - (srcAddr & ((1UL << BITS_4K) - 1));
    if (MapTableIsBlock(&wl->mapTable, dstAddr))
        dstCanWriteSize = SIZE_2M - (dstAddr & ((1UL << BITS_2M) - 1));
    else
        dstCanWriteSize = SIZE_4K - (dstAddr & ((1UL << BITS_4K) - 1));

    copySize = srcCanReadSize < dstCanWriteSize ? srcCanReadSize : dstCanWriteSize;
    copySize = copySize < size ? copySize : size;

    if (MapTableIsBlock(&wl->mapTable, srcAddr))
        BlockManagerRead(&wl->blockManager, srcAddr, copySize, tmpBuffer);
    else
        PageManagerRead(&wl->pageManager, srcAddr, copySize, tmpBuffer);

    if (MapTableIsBlock(&wl->mapTable, dstAddr))
        BlockManagerWrite(&wl->blockManager, dstAddr, copySize, tmpBuffer,
                          copySize == dstCanWriteSize ? increasedWearCount : 0);
    else
        PageManagerWrite(&wl->pageManager, dstAddr, copySize, tmpBuffer,
                         copySize == dstCanWriteSize ? increasedWearCount : 0);

    return copySize;
}

void WearLevelerTrim(struct WearLeveler * wl, logic_addr_t addr)
{
    if (MapTableIsBlock(&wl->mapTable, addr))
        BlockManagerTrim(&wl->blockManager, addr);
    else
        PageManagerTrim(&wl->pageManager, addr);
}

nvm_addr_t WearLevelerReserveDataAddrQuery(struct WearLeveler * wl)
{
    return wl->layouter.reserveData;
}

UINT64 WearLevelerLogicBlockNumQuery(struct WearLeveler * wl)
{
    return BlockNumQuery(&wl->layouter) - LOGIC_BLOCK_RESERVE_NUM;
}

nvm_addr_t WearLevelerDataStartAddrQuery(struct WearLeveler * wl)
{
    return wl->layouter.dataStart;
}