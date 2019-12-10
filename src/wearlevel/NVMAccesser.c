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
    UINT64 readSize = 0;
    while (readSize < size)
    {
        UINT64 thisTimeReadSize;
        thisTimeReadSize = WearLevelerRead(acc->wl, addr, buffer, size);
        readSize += thisTimeReadSize;
        addr += thisTimeReadSize;
    }
    return readSize;
}

int NVMAccesserWrite(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer, int increaseWearCount)
{
    UINT64 writeSize = 0;
    while (writeSize < size)
    {
        UINT64 thisTimeWriteSize;

        thisTimeWriteSize = WearLevelerWrite(acc->wl, addr, buffer, size, increaseWearCount);
        addr += thisTimeWriteSize;
        writeSize += thisTimeWriteSize;
    }
    return writeSize;
}

int NVMAccesserMemset(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, int value, int increaseWearCount)
{
    UINT64 setSize = 0;
    while (setSize < size)
    {
        UINT64 thisTimeSetSize;

        thisTimeSetSize = WearLevelerMemset(acc->wl, addr, size, value, increaseWearCount);
        addr += thisTimeSetSize;
        setSize += thisTimeSetSize;
    }
    return setSize;
}

int NVMAccesserMemcpy(struct NVMAccesser * acc, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT64 size,
                      int increaseWearCount)
{
    UINT64 copySize = 0;
    while (copySize < size)
    {
        UINT64 thisTimeCopySize = WearLevelerMemcpy(acc->wl, srcAddr, dstAddr, size, increaseWearCount);
        copySize += thisTimeCopySize;
        srcAddr += thisTimeCopySize;
        dstAddr += thisTimeCopySize;
    }
    return copySize;
}

void NVMAccesserTrim(struct NVMAccesser * acc, logic_addr_t addr)
{
    WearLevelerTrim(acc->wl, addr);
}