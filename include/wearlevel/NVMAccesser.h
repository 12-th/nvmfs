#ifndef NVM_ACCESSER_H
#define NVM_ACCESSER_H

#include "Types.h"

struct WearLeveler;

struct NVMAccesserFlags
{
    UINT64 pageOrBlock : 1;
};

struct NVMAccesser
{
    logic_addr_t addr;
    struct NVMAccesserFlags flags;
    struct WearLeveler * wl;
};

void NVMAccesserInit(struct NVMAccesser * acc, struct WearLeveler * wl, logic_addr_t addr,
                     enum NVMAccesserRangeSize type);
void NVMAccesserRead(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer);
void NVMAccesserWrite(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer);

#endif