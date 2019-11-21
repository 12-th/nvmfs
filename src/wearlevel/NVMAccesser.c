#include "NVMAccesser.h"
#include "NVMOperations.h"
#include "WearLeveler.h"

void NVMAccesserInit(struct NVMAccesser * acc, struct WearLeveler * wl)
{
    acc->wl = wl;
}

void NVMAccesserUninit(struct NVMAccesser * acc)
{
}

void NVMAccesserSplit(struct NVMAccesser * acc, logic_addr_t addr)
{
    NVMBlockSplit(acc->wl, addr);
}

void NVMAccesserMerge(struct NVMAccesser * acc, logic_addr_t addr)
{
    NVMPagesMerge(acc->wl, addr);
}

int NVMAccesserRead(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer)
{
    return WearLevelerRead(acc->wl, addr, buffer, size);
}

int NVMAccesserWrite(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer, int increaseWearCount)
{
    return WearLevelerWrite(acc->wl, addr, buffer, size, increaseWearCount);
}

void NVMAccesserTrim(struct NVMAccesser * acc, logic_addr_t addr)
{
    WearLevelerTrim(acc->wl, addr);
}