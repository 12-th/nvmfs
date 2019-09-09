#ifndef LAYOUTER_H
#define LAYOUTER_H

#include "NVMPtr.h"

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
    nvm_addr_t blockMetaDataTable;
    nvm_addr_t availBlockTable;

    nvm_addr_t blockWearTable;
    nvm_addr_t blockUnmapTable;

    nvm_addr_t pageWearTable;
    nvm_addr_t pageUnmapTable;

    nvm_addr_t mapTable;

    nvm_addr_t swapTable;

    nvm_addr_t dataStart;
};

void LayouterInit(struct Layouter * l, UINT64 nvmSizeBits);

#endif