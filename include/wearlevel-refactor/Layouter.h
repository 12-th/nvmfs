#ifndef LAYOUTER_H
#define LAYOUTER_H

#include "Config.h"
#include "Types.h"
#include "common.h"

struct Layouter
{
    UINT64 nvmSizeBits;
    UINT64 reserveDataSize;

    logic_addr_t blockSwapLogLogicAddr;
    logic_addr_t pageSwapLogLogicAddr;
    logic_addr_t pageSwapAreaLogicAddr;

    nvm_addr_t superBlock;
    nvm_addr_t metaDataOfThreshold;
    nvm_addr_t metaDataOfBlockSwapLog;
    nvm_addr_t metaDataOfPageSwapLog;
    nvm_addr_t metaDataOfPageSwapArea;

    nvm_addr_t blockWearTable;
    nvm_addr_t blockUnmapTable;

    nvm_addr_t pageWearTable;
    nvm_addr_t pageUnmapTable;

    nvm_addr_t reserveData;

    nvm_addr_t dataStart;
};

void LayouterInit(struct Layouter * l, UINT64 nvmSizeBits, UINT64 reserveDataSize);
static inline physical_block_t nvm_addr_to_block(nvm_addr_t addr, UINT64 dataStart)
{
    return (addr - dataStart) >> BITS_2M;
}

static inline nvm_addr_t block_to_nvm_addr(physical_block_t block, UINT64 dataStart)
{
    return (block << BITS_2M) + dataStart;
}

static inline ALWAYS_INLINE physical_page_t nvm_addr_to_page(nvm_addr_t addr, UINT64 dataStart)
{
    return (addr - dataStart) >> BITS_4K;
}

static inline ALWAYS_INLINE nvm_addr_t page_to_nvm_addr(physical_page_t page, UINT64 dataStart)
{
    return (page << BITS_4K) + dataStart;
}

static inline ALWAYS_INLINE physical_block_t page_to_block(physical_page_t page)
{
    return page >> (BITS_2M - BITS_4K);
}

static inline ALWAYS_INLINE physical_page_t block_to_page(physical_block_t block)
{
    return block << (BITS_2M - BITS_4K);
}

static inline ALWAYS_INLINE UINT64 BlockNumQuery(struct Layouter * l)
{
    return ((1UL << l->nvmSizeBits) - l->dataStart) >> BITS_2M;
}

static inline ALWAYS_INLINE UINT64 PageNumQuery(struct Layouter * l)
{
    return ((1UL << l->nvmSizeBits) - l->dataStart) >> BITS_4K;
}

static inline ALWAYS_INLINE UINT64 NVMSizeQuery(struct Layouter * l)
{
    return 1UL << l->nvmSizeBits;
}

#endif