#include "Align.h"
#include "ContinousSpace.h"
#include "FsConstructor.h"
#include "PagePool.h"
#include <linux/slab.h>

static inline UINT64 RestSizeOfCurrentArea(logic_addr_t readWriteStart)
{
    return PAGE_SIZE - (readWriteStart & (PAGE_SIZE - 1));
}

static inline logic_addr_t CurrentAreaOfAddr(logic_addr_t addr)
{
    return addr & (~(PAGE_SIZE - 1));
}

int ContinousSpaceFormat(struct ContinousSpace * cs, struct PagePool * pool)
{
    logic_addr_t addr;

    addr = PagePoolAlloc(pool);
    if (addr == invalid_nvm_addr)
        return -ENOSPC;

    cs->firstArea = addr;
    cs->areaNum = 1;
    cs->restSize = PAGE_SIZE - sizeof(logic_addr_t);
    return 0;
}

void ContinousSpaceUninit(struct ContinousSpace * cs)
{
}

void ContinousSpaceDestroy(struct ContinousSpace * cs, struct PagePool * pool, struct NVMAccesser * acc)
{
    logic_addr_t cur, next;
    DEBUG_PRINT("log destroy, areas are :\n");
    cur = cs->firstArea;
    while (cs->areaNum)
    {
        DEBUG_PRINT("0x%lx\t", cur);
        NVMAccesserRead(acc, cur, sizeof(logic_addr_t), &next);
        PagePoolFree(pool, cur);
        cur = next;
        cs->areaNum--;
    }
    DEBUG_PRINT("\n");
}

static logic_addr_t * AllocAreas(struct ContinousSpace * cs, UINT64 pageNum, struct PagePool * pool,
                                 logic_addr_t curArea, int * err)
{
    logic_addr_t * array;
    int i = 0;
    logic_addr_t lastPage = curArea;

    *err = 0;

    if (pageNum <= CONTINOUS_SPACE_INLINE_ARRAY_SIZE)
        array = cs->inlineArray;
    else
        array = kmalloc(sizeof(logic_addr_t) * pageNum, GFP_KERNEL);
    if (array == NULL)
    {
        *err = -ENOMEM;
        return NULL;
    }

    for (i = 0; i < pageNum; ++i)
    {
        array[i] = PagePoolAllocWithHint(pool, lastPage);
        if (array[i] == invalid_nvm_addr)
            goto failed;
        lastPage = array[i];
    }

    return array;

failed:
    for (i--; i >= 0; i--)
    {
        PagePoolFree(pool, array[i]);
    }
    if (array != cs->inlineArray)
        kfree(array);
    *err = -ENOSPC;
    return NULL;
}

static void LinkAreas(struct ContinousSpace * cs, logic_addr_t * arr, UINT64 num, struct NVMAccesser * acc,
                      logic_addr_t start)
{
    int i;
    logic_addr_t lastArea = CurrentAreaOfAddr(start);

    for (i = 0; i < num; ++i)
    {
        NVMAccesserWrite(acc, lastArea, sizeof(logic_addr_t), &arr[i], 0);
        lastArea = arr[i];
    }
    cs->areaNum += num;
    cs->restSize += num * (PAGE_SIZE - sizeof(logic_addr_t));
}

int ContinousSpacePreallocSpace(struct ContinousSpace * cs, UINT64 size, struct PagePool * pool,
                                struct NVMAccesser * acc, logic_addr_t start)
{
    UINT64 pageNum = AlignUp(size, (SIZE_4K - sizeof(logic_addr_t))) / (SIZE_4K - sizeof(logic_addr_t));
    logic_addr_t * array;
    int err;

    array = AllocAreas(cs, pageNum, pool, CurrentAreaOfAddr(start), &err);
    if (array == NULL)
        return err;
    LinkAreas(cs, array, pageNum, acc, start);
    if (array != cs->inlineArray)
        kfree(array);
    return 0;
}

int ContinousSpaceEssureEnoughSpace(struct ContinousSpace * cs, logic_addr_t addr, UINT64 size, struct PagePool * pool,
                                    struct NVMAccesser * acc)
{
    int err;
    if (cs->restSize >= size)
    {
        return 0;
    }
    err = ContinousSpacePreallocSpace(cs, size - cs->restSize, pool, acc, addr);
    return err;
}

void ContinousSpaceNotifyUsedSpace(struct ContinousSpace * cs, UINT64 size)
{
    cs->restSize -= size;
}

static UINT64 WriteDataToCurrentArea(void * buffer, UINT64 size, struct NVMAccesser * acc, int * full,
                                     logic_addr_t addr)
{
    UINT64 restSize = RestSizeOfCurrentArea(addr);
    UINT64 writeSize = size <= restSize ? size : restSize;

    *full = !!(writeSize == restSize);
    if (restSize == 0)
        return 0;
    NVMAccesserWrite(acc, addr, writeSize, buffer, *full);
    return writeSize;
}

void ContinousSpaceWrite(void * buffer, UINT64 size, logic_addr_t addr, struct NVMAccesser * acc)
{
    do
    {
        int full;
        UINT64 writeSize;

        writeSize = WriteDataToCurrentArea(buffer, size, acc, &full, addr);
        size -= writeSize;
        buffer += writeSize;
        if (size && full)
        {
            logic_addr_t next;

            NVMAccesserRead(acc, CurrentAreaOfAddr(addr), sizeof(logic_addr_t), &next);
            addr = next + sizeof(logic_addr_t);
        }
    } while (size);
}

void ContinousSpaceRead(void * buffer, UINT64 size, logic_addr_t start, struct NVMAccesser * acc)
{
    do
    {
        UINT64 restSize;
        UINT64 readSize;

        restSize = RestSizeOfCurrentArea(start);
        readSize = restSize >= size ? size : restSize;
        NVMAccesserRead(acc, start, readSize, buffer);
        buffer += readSize;
        size -= readSize;
        if (size)
        {
            logic_addr_t next, curArea;

            curArea = CurrentAreaOfAddr(start);
            NVMAccesserRead(acc, curArea, sizeof(logic_addr_t), &next);
            start = next + sizeof(logic_addr_t);
        }
    } while (size);
}

logic_addr_t ContinousSpaceCalculateNextAddr(logic_addr_t start, UINT64 size, struct NVMAccesser * acc)
{
    do
    {
        UINT64 restSize;

        restSize = RestSizeOfCurrentArea(start);
        if (restSize > size)
            return start + size;
        else
        {
            logic_addr_t next, curArea;
            curArea = CurrentAreaOfAddr(start);
            NVMAccesserRead(acc, curArea, sizeof(logic_addr_t), &next);
            start = next + sizeof(logic_addr_t);
            size -= restSize;
        }
    } while (1);

    return 0;
}

void ContinousSpaceRecoveryInit(struct ContinousSpace * cs, logic_addr_t firstAreaAddr)
{
    cs->firstArea = firstAreaAddr;
}

void ContinousSpaceRecovery(struct ContinousSpace * cs, logic_addr_t lastAddr, struct FsConstructor * ctor,
                            struct NVMAccesser * acc)
{
    logic_addr_t lastArea = CurrentAreaOfAddr(lastAddr);
    logic_addr_t cur, next;
    cur = cs->firstArea;
    while (cur != lastArea)
    {
        NVMAccesserRead(acc, cur, sizeof(logic_addr_t), &next);
        FsConstructorNotifyPageBusy(ctor, cur);
        cur = next;
    }
    FsConstructorNotifyPageBusy(ctor, cur);
}

void ContinousSpaceRecoveryUninit(struct ContinousSpace * cs)
{
}

void ContinousSpaceRebuildBegin(struct ContinousSpace * cs, logic_addr_t firstArea)
{
    cs->firstArea = firstArea;
}

void ContinousSpaceRebuildEnd(struct ContinousSpace * cs, logic_addr_t lastAddr, struct NVMAccesser * acc)
{
    logic_addr_t lastArea = CurrentAreaOfAddr(lastAddr);
    logic_addr_t cur, next;

    cs->areaNum = 1;
    cur = cs->firstArea;
    while (cur != lastArea)
    {
        NVMAccesserRead(acc, cur, sizeof(logic_addr_t), &next);
        cur = next;
        cs->areaNum++;
    }
    cs->restSize = RestSizeOfCurrentArea(lastAddr);
}