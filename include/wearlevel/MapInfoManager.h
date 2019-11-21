#ifndef MAP_INFO_MANAGER_H
#define MAP_INFO_MANAGER_H

#include "BlockUnmapTable.h"
#include "NVMAccessController.h"
#include "PageUnmapTable.h"
#include "Types.h"

struct Layouter;
struct PageWearTable;
struct BlockWearTable;

struct BlockMapInfo
{
    physical_block_t physBlock;
    struct BlockInfo blockInfo;
};

struct PageMapInfo
{
    physical_page_t physPage;
    struct PageInfo pageInfo;
};

struct MapInfoManager
{
    struct BlockUnmapTable blockUnmapTable;
    struct PageUnmapTable pageUnmapTable;
    struct NVMAccessController nvmAccessController;
    nvm_addr_t dataStartOffset;
};

void MapInfoManagerFormat(struct MapInfoManager * manager, struct Layouter * layouter);
void MapInfoManagerInit(struct MapInfoManager * manager, struct Layouter * layouter);
void MapInfoManagerUninit(struct MapInfoManager * manager);

void PageMapInfoBuildFromLogicalPage(struct PageMapInfo * info, struct MapInfoManager * manager, logical_page_t page);
void BlockMapInfoBuildFromLogicalBlock(struct BlockMapInfo * info, struct MapInfoManager * manager,
                                       logical_block_t block);

void BlockWearCountIncreasePrepare(struct MapInfoManager * manager, logic_addr_t addr);
void BlockWearCountIncreaseEnd(struct MapInfoManager * manager, logic_addr_t addr);
void BlockSwapPrepare(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                      struct BlockMapInfo * newBlockInfo, physical_block_t oldBlock, physical_block_t newBlock);
void BlockSwapEnd(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                  struct BlockMapInfo * newBlockInfo);
void PageWearCountIncreasePrepare(struct MapInfoManager * manager, logic_addr_t addr);
void PageWearCountIncreaseEnd(struct MapInfoManager * manager, logic_addr_t addr);
void PageWearCountIncreaseToSwapPrepare(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo,
                                        struct PageMapInfo * newPageInfo, physical_page_t oldPage,
                                        physical_page_t newPage);
void PageSwapEnd(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo, struct PageMapInfo * newPageInfo);

void MapInfoManagerSwapBlock(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                             struct BlockMapInfo * newBlockInfo);
void MapInfoManagerSwapPage(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo,
                            struct PageMapInfo * newPageInfo);

UINT32 MapInfoManagerRead(struct MapInfoManager * manager, logic_addr_t addr, void * buffer, UINT32 size);
UINT32 MapInfoManagerWrite(struct MapInfoManager * manager, logic_addr_t addr, void * buffer, UINT32 size);
void MapInfoManagerTrim(struct MapInfoManager * manager, logic_addr_t addr);
void MapInfoManagerSplit(struct MapInfoManager * manager, logic_addr_t addr, struct BlockWearTable * blockWearTable,
                         struct PageWearTable * pageWearTable);
void MapInfoManagerMerge(struct MapInfoManager * manager, logic_addr_t addr, struct BlockWearTable * blockWearTable,
                         struct PageWearTable * pageWearTable);

int MapInfoManagerIsBlockSplited(struct MapInfoManager * manager, logic_addr_t addr);

#endif