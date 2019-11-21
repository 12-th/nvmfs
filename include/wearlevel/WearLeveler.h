#ifndef WEAR_LEVELER_H
#define WEAR_LEVELER_H

#include "AvailBlockTable.h"
#include "AvailPageTable.h"
#include "BlockSwapTransactionLogArea.h"
#include "BlockUnmapTable.h"
#include "BlockWearTable.h"
#include "Layouter.h"
#include "MapInfoManager.h"
#include "NVMAccessController.h"
#include "PageSwapTransactionLogArea.h"
#include "PageUnmapTable.h"
#include "PageWearTable.h"
#include "SuperBlock.h"
#include "SwapTable.h"

struct WearLeveler
{
    UINT64 nvmBaseAddr;
    struct SuperBlock sb;
    struct Layouter layouter;
    struct BlockWearTable blockWearTable;
    struct PageWearTable pageWearTable;
    struct AvailBlockTable availBlockTable;
    struct AvailPageTable availPageTable;
    struct SwapTable swapTable;
    struct BlockSwapTransactionLogArea blockSwapTransactionLogArea;
    struct PageSwapTransactionLogArea pageSwapTransactionLogArea;
    struct MapInfoManager mapInfoManager;
};

void WearLevelerInit(struct WearLeveler * wl);
void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 nvmBaseAddr);
void WearLevelerUninit(struct WearLeveler * wl);
void NVMBlockSplit(struct WearLeveler * wl, logic_addr_t addr);
void NVMPagesMerge(struct WearLeveler * wl, logic_addr_t addr);
int WearLevelerRead(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size);
UINT32 WearLevelerWrite(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size,
                        UINT32 increasedWearCount);
void WearLevelerTrim(struct WearLeveler * wl, logic_addr_t addr);

#endif