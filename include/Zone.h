#ifndef ZONE_H
#define ZONE_H

#include "Config.h"
#include "SuperBlock.h"

struct Zone
{
    block_t availBlocks[ZONE_AVAIL_BLOCK_CACHE_NUM];
    UINT32 blockNum;
    struct SuperBlock * sb;
};

// void ZoneInit(struct Zone * zone, struct SuperBlock * sb);
// block_t ZoneGetAvailBlocks(struct Zone * zone);

#endif