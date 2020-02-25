#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "PageSwapArea.h"
#include "Types.h"

struct BlockManager;
struct PageUnmapTable;
struct BlockUnmapTable;
struct PageWearTable;
struct PageSwapLog;
struct PageColdnessManager;
struct PageSwapArea;
struct MapTable;
struct WearLeveler;

struct PageManager
{
    UINT64 dataStartOffset;
    struct BlockManager * blockManager;
    struct PageUnmapTable * unmapTable;
    struct BlockUnmapTable * blockUnmapTable;
    struct PageWearTable * wearTable;
    struct PageSwapLog * log;
    struct PageColdnessManager * coldness;
    struct PageSwapArea * swapArea;
    struct MapTable * mapTable;
    logic_addr_t swapLogLogicAddr;
    logic_addr_t swapAreaLogicAddr;
};

void PageManagerInit(struct PageManager * manager, struct WearLeveler * wl);
void PageManagerRead(struct PageManager * manager, logic_addr_t addr, UINT64 size, void * buffer);
void PageManagerWrite(struct PageManager * manager, logic_addr_t addr, UINT64 size, void * buffer,
                      int increaseWearCount);
void PageManagerMemset(struct PageManager * manager, logic_addr_t addr, UINT64 size, int value, int increaseWearCount);
void PageManagerTrim(struct PageManager * manager, logic_addr_t addr);

#endif