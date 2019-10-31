#ifndef NVM_ACCESSER_H
#define NVM_ACCESSER_H

#include "Types.h"

struct WearLeveler;

struct NVMAccesserFlags
{
    UINT64 pageOrBlock : 1;
    UINT64 physAddrVaildFlag : 1;
};

enum NVMAccesserRangeType
{
    NVM_ACCESSER_BLOCK,
    NVM_ACCESSER_PAGE
};

struct NVMAccesser
{
    logic_addr_t addr;
    nvm_addr_t physAddr;
    struct NVMAccesserFlags flags;
    struct WearLeveler * wl;
};

void NVMAccesserInit(struct NVMAccesser * acc, struct WearLeveler * wl, logic_addr_t addr,
                     enum NVMAccesserRangeSize type);
void NVMAccesserRead(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer);
void NVMAccesserWrite(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer);

#endif