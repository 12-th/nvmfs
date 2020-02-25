#include "Align.h"
#include "Config.h"
#include "Layouter.h"
#include "PageUnmapTable.h"

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

static inline UINT64 sizeOfReservedSpace(void)
{
    return SIZE_2M;
}

void LayouterInit(struct Layouter * l, UINT64 nvmSizeBits, UINT64 reserveDataSize)
{
    l->nvmSizeBits = nvmSizeBits;
    l->reserveDataSize = reserveDataSize;

    l->superBlock = 0;
    l->metaDataOfThreshold = 1UL << 12;
    l->metaDataOfBlockSwapLog = l->metaDataOfThreshold + sizeof(nvm_addr_t);
    l->metaDataOfPageSwapLog = l->metaDataOfBlockSwapLog + sizeof(nvm_addr_t);
    l->metaDataOfPageSwapLog = l->metaDataOfPageSwapLog + sizeof(nvm_addr_t);
    l->metaDataOfPageSwapArea = l->metaDataOfPageSwapLog + sizeof(nvm_addr_t);

    l->blockWearTable = sizeOfReservedSpace();
    l->blockUnmapTable = l->blockWearTable + sizeOfBlockWearTable(nvmSizeBits);
    l->pageWearTable = l->blockUnmapTable + sizeOfBlockUnmapTable(nvmSizeBits);
    l->pageUnmapTable = l->pageWearTable + sizeOfPageWearTable(nvmSizeBits);
    l->reserveData = l->pageUnmapTable + sizeofPageUnmapTable(nvmSizeBits);
    l->dataStart = AlignUpBits(l->reserveData + l->reserveDataSize, BITS_2M);

    l->blockSwapLogLogicAddr = (BlockNumQuery(l) - LOGIC_BLOCK_RESERVE_NUM) * SIZE_2M;
    l->pageSwapLogLogicAddr = l->blockSwapLogLogicAddr + SIZE_2M;
    l->pageSwapAreaLogicAddr = l->pageSwapLogLogicAddr + SIZE_2M;
}