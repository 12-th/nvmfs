#ifndef WEAR_LEVELER_H
#define WEAR_LEVELER_H

#include "BlockColdnessManager.h"
#include "BlockManager.h"
#include "BlockSwapLog.h"
#include "BlockUnmapTable.h"
#include "BlockWearTable.h"
#include "Layouter.h"
#include "MapTable.h"
#include "PageColdnessManager.h"
#include "PageManager.h"
#include "PageSwapArea.h"
#include "PageSwapLog.h"
#include "PageUnmapTable.h"
#include "PageWearTable.h"

struct WearLeveler
{
    struct Layouter layouter;
    struct MapTable mapTable;

    struct BlockColdnessManager * blockColdnessManager;
    struct BlockSwapLog blockSwapLog;
    struct BlockUnmapTable blockUnmapTable;
    struct BlockWearTable blockWearTable;

    struct PageColdnessManager pageColdnessManager;
    struct PageSwapArea pageSwapArea;
    struct PageSwapLog pageSwapLog;
    struct PageUnmapTable pageUnmapTable;
    struct PageWearTable pageWearTable;

    struct BlockManager blockManager;
    struct PageManager pageManager;
};

void WearLevelerInit(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize);
void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize);
void WearLevelerUninit(struct WearLeveler * wl);
void NVMBlockSplit(struct WearLeveler * wl, logic_addr_t addr);
void NVMPagesMerge(struct WearLeveler * wl, logic_addr_t addr);
UINT32 WearLevelerRead(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size);
UINT32 WearLevelerWrite(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size,
                        UINT32 increasedWearCount);
UINT32 WearLevelerMemset(struct WearLeveler * wl, logic_addr_t addr, UINT32 size, int value, UINT32 increasedWearCount);
UINT32 WearLevelerMemcpy(struct WearLeveler * wl, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT32 size,
                         UINT32 increasedWearCount, void * tmpBuffer);
void WearLevelerTrim(struct WearLeveler * wl, logic_addr_t addr);

nvm_addr_t WearLevelerReserveDataAddrQuery(struct WearLeveler * wl);
UINT64 WearLevelerLogicBlockNumQuery(struct WearLeveler * wl);
nvm_addr_t WearLevelerDataStartAddrQuery(struct WearLeveler * wl);

#endif