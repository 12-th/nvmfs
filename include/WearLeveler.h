#ifndef WEAR_LEVELER_H
#define WEAR_LEVELER_H

#include "AvailBlockTable.h"
#include "BlockSwapTransactionLogArea.h"
#include "BlockUnmapTable.h"
#include "BlockWearTable.h"
#include "Layouter.h"
#include "MapTable.h"
#include "PageUnmapTable.h"
#include "PageWearTable.h"
#include "SuperBlock.h"
#include "SwapTable.h"

struct WearLeveler
{
    UINT64 nvmBaseAddr;
    struct SuperBlock sb;
    struct Layouter layouter;
    struct MapTable mapTable;
    struct BlockUnmapTable blockUnmapTable;
    struct BlockWearTable blockWearTable;
    struct PageUnmapTable pageUnmapTable;
    struct PageWearTable pageWearTable;
    struct AvailBlockTable availBlockTable;
    struct SwapTable swapTable;
    struct BlockSwapTransactionLogArea blockSwapTransactionLogArea;
};

void WearLevelerInit(struct WearLeveler * wl);
void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 nvmBaseAddr);
void WearLevelerUninit(struct WearLeveler * wl);
nvm_addr_t LogicAddressTranslate(struct WearLeveler * wl, logic_addr_t addr);
void NVMBlockWearCountIncrease(struct WearLeveler * wl, logic_addr_t addr, UINT32 delta);
void NVMPageWearCountIncrease(struct WearLeveler * wl, logic_addr_t addr, UINT32 delta);

#endif