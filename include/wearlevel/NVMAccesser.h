#ifndef NVM_ACCESSER_H
#define NVM_ACCESSER_H

#include "Types.h"

struct NVMAccesser
{
    struct WearLeveler * wl;
};

void NVMAccesserInit(struct NVMAccesser * acc, struct WearLeveler * wl);
void NVMAccesserUninit(struct NVMAccesser * acc);
void NVMAccesserSplit(struct NVMAccesser * acc, logic_addr_t addr);
void NVMAccesserMerge(struct NVMAccesser * acc, logic_addr_t addr);
int NVMAccesserRead(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer);
int NVMAccesserWrite(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer, int increaseWearCount);
int NVMAccesserMemset(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, int value, int increaseWearCount);
int NVMAccesserMemcpy(struct NVMAccesser * acc, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT64 size,
                      int increaseWearCount);
void NVMAccesserTrim(struct NVMAccesser * acc, logic_addr_t addr);

#endif