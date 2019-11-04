#ifndef WEAR_COUNT_CACULATE_H
#define WEAR_COUNT_CACULATE_H

#include "Config.h"
#include "Types.h"

static inline int IsCrossWearCountThreshold(UINT32 oldWearCount, UINT32 newWearCount)
{
    return !(oldWearCount / STEP_WEAR_COUNT == newWearCount / STEP_WEAR_COUNT);
}

static inline UINT32 NextWearCountThreshold(UINT32 wearCount)
{
    return wearCount - (wearCount % STEP_WEAR_COUNT) + STEP_WEAR_COUNT;
}

#endif