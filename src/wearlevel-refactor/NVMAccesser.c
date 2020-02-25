#include "NVMAccesser.h"
#include "NVMOperations.h"
#include "WearLeveler.h"
#include "common.h"
#include <linux/mm.h>
#include <linux/slab.h>

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
    while (size)
    {
        UINT64 thisTimeReadSize;
        thisTimeReadSize = WearLevelerRead(acc->wl, addr, buffer, size);
        readSize += thisTimeReadSize;
        addr += thisTimeReadSize;
        size -= thisTimeReadSize;
    }
    return readSize;
}

int NVMAccesserWrite(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, void * buffer, int increaseWearCount)
{
    UINT64 writeSize = 0;
    while (size)
    {
        UINT64 thisTimeWriteSize;

        thisTimeWriteSize = WearLevelerWrite(acc->wl, addr, buffer, size, increaseWearCount);
        addr += thisTimeWriteSize;
        writeSize += thisTimeWriteSize;
        size -= thisTimeWriteSize;
    }
    return writeSize;
}

int NVMAccesserMemset(struct NVMAccesser * acc, logic_addr_t addr, UINT64 size, int value, int increaseWearCount)
{
    UINT64 setSize = 0;
    while (size)
    {
        UINT64 thisTimeSetSize;

        thisTimeSetSize = WearLevelerMemset(acc->wl, addr, size, value, increaseWearCount);
        addr += thisTimeSetSize;
        setSize += thisTimeSetSize;
        size -= thisTimeSetSize;
    }
    return setSize;
}

int NVMAccesserMemcpy(struct NVMAccesser * acc, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT64 size,
                      int increaseWearCount)
{
    UINT64 copySize = 0;
    void * buffer;

    buffer = kvmalloc(SIZE_2M, GFP_KERNEL);
    while (size)
    {
        UINT64 thisTimeCopySize = WearLevelerMemcpy(acc->wl, srcAddr, dstAddr, size, increaseWearCount, buffer);
        copySize += thisTimeCopySize;
        srcAddr += thisTimeCopySize;
        dstAddr += thisTimeCopySize;
        size -= thisTimeCopySize;
    }
    kvfree(buffer);
    return copySize;
}

void NVMAccesserTrim(struct NVMAccesser * acc, logic_addr_t addr)
{
    WearLevelerTrim(acc->wl, addr);
}