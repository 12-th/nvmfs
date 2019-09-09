#include "Align.h"
#include "Config.h"
#include "Layouter.h"

static inline UINT64 sizeOfBlockMetaDataTable(UINT64 nvmSizeBits)
{
    return alignUpBits((1UL << (nvmSizeBits - BITS_2M)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfAvailBlockTable(void)
{
    return alignUpBits((MAX_WEAR_COUNT / STEP_WEAR_COUNT) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfBlockWearTable(UINT64 nvmSizeBits)
{
    return alignUpBits((1UL << (nvmSizeBits - BITS_2M)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfBlockUnmapTable(UINT64 nvmSizeBits)
{
    return alignUpBits((1UL << (nvmSizeBits - BITS_2M)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfPageWearTable(UINT64 nvmSizeBits)
{
    return alignUpBits((1UL << (nvmSizeBits - BITS_4K)) * sizeof(UINT32), BITS_2M);
}

static inline UINT64 sizeOfPageUnmapTable(UINT64 nvmSizeBits)
{
    return alignUpBits(alignUp((1UL << (nvmSizeBits - BITS_4K)), 7) / 7 * 8, BITS_2M);
}

static inline UINT64 sizeOfMapTable(UINT64 nvmSizeBits)
{
    UINT64 pmdSize;
    UINT64 pudSize;
    UINT64 pgdSize;

    if (nvmSizeBits <= BITS_2M)
    {
        pmdSize = SIZE_4K;
    }
    else
    {
        pmdSize = alignUpBits(1UL << (nvmSizeBits - BITS_2M + 3), BITS_4K);
    }

    if (nvmSizeBits <= 30)
    {
        pudSize = SIZE_4K;
    }
    else
    {
        pudSize = alignUpBits(1UL << (nvmSizeBits - 30 + 3), BITS_4K);
    }

    if (nvmSizeBits <= 39)
    {
        pudSize = SIZE_4K;
    }
    else
    {
        pgdSize = alignUpBits(1UL << (nvmSizeBits - 39 + 3), BITS_4K);
    }

    return alignUpBits(pmdSize + pudSize + pgdSize, BITS_2M);
}

static inline UINT64 sizeOfSwapTable(void)
{
    return 256UL << 20;
}

static inline UINT64 sizeOfReservedSpace(void)
{
    return SIZE_2M;
}

void LayouterInit(struct Layouter * l, UINT64 nvmSizeBits)
{
    l->nvmSizeBits = nvmSizeBits;
    l->blockMetaDataTable = nvm_addr_from_ul(sizeOfReservedSpace());
    l->availBlockTable = nvm_ptr_advance(l->blockMetaDataTable, sizeOfBlockMetaDataTable(nvmSizeBits));
    l->blockWearTable = nvm_ptr_advance(l->availBlockTable, sizeOfAvailBlockTable());
    l->blockUnmapTable = nvm_ptr_advance(l->blockWearTable, sizeOfBlockWearTable(nvmSizeBits));
    l->pageWearTable = nvm_ptr_advance(l->blockUnmapTable, sizeOfBlockUnmapTable(nvmSizeBits));
    l->pageUnmapTable = nvm_ptr_advance(l->pageWearTable, sizeOfPageWearTable(nvmSizeBits));
    l->mapTable = nvm_ptr_advance(l->pageUnmapTable, sizeOfPageUnmapTable(nvmSizeBits));
    l->swapTable = nvm_ptr_advance(l->mapTable, sizeOfMapTable(nvmSizeBits));
    l->dataStart = nvm_ptr_advance(l->swapTable, sizeOfSwapTable());
}