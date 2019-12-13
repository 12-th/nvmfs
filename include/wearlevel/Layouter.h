#ifndef LAYOUTER_H
#define LAYOUTER_H

#include "Config.h"
#include "Types.h"
#include "common.h"

// 需要记录的信息
// 总大小，n个比特位（1T对应40位） nvmSizeBits
// 总阈值 2^32， 每梯度阈值 2^10，共2^22种

// 2M块的元数据信息 需(n-21)位，分配32位足够 blockMetaDataTable
// 按照阈值梯度将块连接起来的链表头表（n-21）*2^22位 availBlockTable

// 2M块的磨损情况 需32位
// 2M块的反向映射情况（物理->逻辑） 每个需要(n-21)位
// 这两项共分配64位，但为了数据一致性考虑，分为两个表blockWearTable和blockUnmapTable
// blockTable

// 4K块的磨损情况 需32位
// 4K块的反向映射情况（物理->逻辑） 每个需要9位，这个每64位容纳7个page的情况，剩下一位不用
// 这两项分开分配，分为两个表
// pageTable

// 映射的情况（逻辑->物理)
// 这个用页表来做，由于限制了4K页只会在它所在的2M范围内运作，所以，在NVM上不记录4K级别的反向映射关系，仅记录2M级别的反向映射关系
// 每2M页需要8字节

// 用于交换的块的数量

// 以1T的NVM为例
// blockMetaDataTable的大小为 1T/2M*4 = 2M
// availBlockTable的大小为 4*2^22= 16M
// blockWearTable的大小为 4*1T/2M = 2M
// blockMapTable的大小为 4*1T/2M = 2M
// pageTable的磨损信息需要 1T/4K*4 = 1G
// pageTable的反向映射信息需要 1T/4K/7*8 = 294M
// mapTable的大小 1T/2M*8 = 4M
// swapTable的大小256M

//算下来，每1T空间，保留2G就够用了

struct Layouter
{
    UINT64 nvmSizeBits;
    UINT64 reserveDataSize;
    nvm_addr_t superBlock;
    nvm_addr_t swapTableMetadata;

    nvm_addr_t blockWearTable;
    nvm_addr_t blockUnmapTable;

    nvm_addr_t pageWearTable;
    nvm_addr_t pageUnmapTable;

    nvm_addr_t swapTable;
    nvm_addr_t blockSwapTransactionLogArea;
    nvm_addr_t pageSwapTransactionLogArea;

    nvm_addr_t mapTableSerializeData;

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

static inline nvm_addr_t DataStartAddrQuery(struct Layouter * l)
{
    return l->dataStart;
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

static inline ALWAYS_INLINE nvm_addr_t BlockWearTableAddrQuery(struct Layouter * l)
{
    return l->blockWearTable;
}

static inline ALWAYS_INLINE nvm_addr_t BlockUnmapTableAddrQuery(struct Layouter * l)
{
    return l->blockUnmapTable;
}

static inline ALWAYS_INLINE nvm_addr_t PageWearTableAddrQuery(struct Layouter * l)
{
    return l->pageWearTable;
}

static inline ALWAYS_INLINE UINT64 PageUnmapTableSizeQuery(struct Layouter * l)
{
    return l->pageUnmapTable - l->pageWearTable;
}

static inline ALWAYS_INLINE nvm_addr_t PageUnmapTableAddrQuery(struct Layouter * l)
{
    return l->pageUnmapTable;
}

static inline ALWAYS_INLINE nvm_addr_t MapTableSerializeDataAddrQuery(struct Layouter * l)
{
    return l->mapTableSerializeData;
}

static inline ALWAYS_INLINE UINT64 SwapTableBlocksNumQuery(struct Layouter * l)
{
    return SWAP_TABLE_BLOCK_NUM;
}

static inline ALWAYS_INLINE UINT64 SwapTablePagesNumQuery(struct Layouter * l)
{
    return SWAP_TABLE_PAGE_NUM;
}

static inline ALWAYS_INLINE nvm_addr_t SwapTableMetadataAddrQuery(struct Layouter * l)
{
    return l->swapTableMetadata;
}

static inline ALWAYS_INLINE nvm_addr_t SwapTableBlocksAddrQuery(struct Layouter * l)
{
    return l->swapTable;
}

static inline ALWAYS_INLINE nvm_addr_t SwapTablePagesAddrQuery(struct Layouter * l)
{
    return l->swapTable + (SWAP_TABLE_BLOCK_NUM << BITS_2M);
}

static inline ALWAYS_INLINE nvm_addr_t BlockSwapTransactionLogAreaAddrQuery(struct Layouter * l)
{
    return l->blockSwapTransactionLogArea;
}

static inline ALWAYS_INLINE nvm_addr_t PageSwapTransactionLogAreaAddrQuery(struct Layouter * l)
{
    return l->pageSwapTransactionLogArea;
}

static inline ALWAYS_INLINE nvm_addr_t ReserveDataAddrQuery(struct Layouter * l)
{
    return l->reserveData;
}

#endif