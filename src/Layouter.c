#include "Align.h"
#include "Config.h"
#include "Layouter.h"
#include "PageUnmapTable.h"
#include "SuperBlock.h"
#include "SwapTable.h"

static inline UINT64 sizeOfBlockWearTable(UINT64 nvmSizeBits)
{
    return AlignUpBits((1UL << (nvmSizeBits - BITS_2M)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfBlockUnmapTable(UINT64 nvmSizeBits)
{
    return AlignUpBits((1UL << (nvmSizeBits - BITS_2M)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeofPageUnmapTable(UINT64 nvmSizeBits)
{
    UINT64 pageNum = 1UL << (nvmSizeBits - BITS_4K);
    UINT64 size = AlignUp(pageNum, 512) / 512 * sizeof(struct NVMPageUnmapTableEntryGroup);
    return AlignUpBits(size, BITS_2M);
}

static inline UINT64 sizeOfPageWearTable(UINT64 nvmSizeBits)
{
    return AlignUpBits((1UL << (nvmSizeBits - BITS_4K)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfMapTableSerializeData(UINT64 nvmSizeBits)
{
    return 0;
}

static inline UINT64 sizeOfSwapTable(void)
{
    return SWAP_TABLE_BLOCK_NUM << BITS_2M;
}

static inline UINT64 sizeOfReservedSpace(void)
{
    return SIZE_2M;
}

static inline UINT64 sizeOfBlockSwapTransactionLogArea(void)
{
    return BLOCK_SWAP_TRANSACTION_LOG_AREA_SIZE;
}

void LayouterInit(struct Layouter * l, UINT64 nvmSizeBits)
{
    l->nvmSizeBits = nvmSizeBits;

    l->superBlock = 0;
    l->swapTableMetadata = sizeof(struct NVMSuperBlock);

    l->blockWearTable = sizeOfReservedSpace();
    l->blockUnmapTable = l->blockWearTable + sizeOfBlockWearTable(nvmSizeBits);
    l->pageWearTable = l->blockUnmapTable + sizeOfBlockUnmapTable(nvmSizeBits);
    l->pageUnmapTable = l->pageWearTable + sizeOfPageWearTable(nvmSizeBits);
    l->swapTable = l->pageUnmapTable + sizeofPageUnmapTable(nvmSizeBits);
    l->blockSwapTransactionLogArea = l->swapTable + sizeOfSwapTable();
    l->mapTableSerializeData = l->blockSwapTransactionLogArea + sizeOfBlockSwapTransactionLogArea();
    l->dataStart = AlignUpBits(l->mapTableSerializeData + sizeOfMapTableSerializeData(nvmSizeBits), BITS_2M);
}