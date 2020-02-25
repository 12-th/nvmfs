#include "Config.h"
#include "FsConstructor.h"
#include "Log.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "common.h"
#include <Align.h>
#include <linux/string.h>

#define LogEntryFoot LogEntryHead

int LogFormat(struct Log * log, struct PagePool * pool, UINT64 reserveSize, struct NVMAccesser * acc)
{
    int err;
    struct LogEntryFoot foot = {.data = 0};

    err = ContinousSpaceFormat(&log->cs, pool);
    if (err)
        return err;
    reserveSize = AlignUp(reserveSize, 8);
    log->reserveSize = AlignUp(reserveSize, 8);
    log->writeStart = ContinousSpaceCalculateNextAddr(ContinousSpaceStart(&log->cs), reserveSize, acc);
    ContinousSpaceNotifyUsedSpace(&log->cs, reserveSize);
    ContinousSpaceWrite(&foot, sizeof(struct LogEntryFoot), log->writeStart, acc);
    return 0;
}

void LogUninit(struct Log * log)
{
    ContinousSpaceUninit(&log->cs);
}

void LogDestroy(struct Log * log, struct NVMAccesser * acc, struct PagePool * pool)
{
    ContinousSpaceDestroy(&log->cs, pool, acc);
}

int LogWrite(struct Log * log, UINT64 size, void * buffer, UINT8 type, logic_addr_t * entryStartAddr,
             struct PagePool * pool, struct NVMAccesser * acc)
{
    UINT64 alignedSize;
    UINT64 headAddr, entryAddr, footAddr;
    struct LogEntryHead head = {.type = type, .size = size};
    struct LogEntryFoot foot = {.data = 0};
    int err;

    alignedSize = AlignUp(size, 8);
    err = ContinousSpaceEssureEnoughSpace(
        &log->cs, log->writeStart, sizeof(struct LogEntryHead) + sizeof(struct LogEntryFoot) + alignedSize, pool, acc);
    ContinousSpaceNotifyUsedSpace(&log->cs, sizeof(struct LogEntryFoot) + alignedSize);
    if (err)
        return err;
    headAddr = log->writeStart;
    entryAddr = ContinousSpaceCalculateNextAddr(headAddr, sizeof(struct LogEntryHead), acc);
    footAddr = ContinousSpaceCalculateNextAddr(entryAddr, alignedSize, acc);
    if (buffer)
    {
        ContinousSpaceWrite(buffer, size, entryAddr, acc);
    }
    ContinousSpaceWrite(&foot, sizeof(struct LogEntryFoot), footAddr, acc);
    ContinousSpaceWrite(&head, sizeof(struct LogEntryHead), headAddr, acc);
    log->writeStart = footAddr;
    if (entryStartAddr)
        *entryStartAddr = entryAddr;
    return 0;
}

void LogWriteReserveData(struct Log * log, UINT64 size, UINT64 offsetInsideReserveData, void * buffer,
                         struct NVMAccesser * acc)
{
    logic_addr_t addr;

    addr = ContinousSpaceCalculateNextAddr(ContinousSpaceStart(&log->cs), offsetInsideReserveData, acc);
    ContinousSpaceWrite(buffer, size, addr, acc);
}

static inline void LogReadImpl(logic_addr_t addr, UINT64 size, void * buffer, struct NVMAccesser * acc)
{
    ContinousSpaceRead(buffer, size, addr, acc);
}

void LogRead(struct Log * log, logic_addr_t addr, UINT64 size, void * buffer, struct NVMAccesser * acc)
{
    (void)(log);
    LogReadImpl(addr, size, buffer, acc);
}

void LogForEachEntryImpl(struct Log * log, struct LogCleanupOps * opsArray, void * data, struct NVMAccesser * acc,
                         logic_addr_t * lastReadAddr)
{
    struct LogEntryHead head;
    logic_addr_t addr;

    addr = ContinousSpaceCalculateNextAddr(ContinousSpaceStart(&log->cs), log->reserveSize, acc);
    ContinousSpaceRead(&head, sizeof(struct LogEntryHead), addr, acc);

    while (head.data != 0)
    {
        UINT64 alignedSize = AlignUp(head.size, 8);
        void * buffer;
        struct LogCleanupOps * ops;

        ops = &opsArray[head.type];
        if (ops->prepare)
        {
            UINT32 size = head.size;
            logic_addr_t readStart, readEnd;

            buffer = ops->prepare(&size, data);
            readStart = ContinousSpaceCalculateNextAddr(addr, sizeof(struct LogEntryHead), acc);
            readEnd = ContinousSpaceCalculateNextAddr(readStart, size, acc);
            ContinousSpaceRead(buffer, size, readStart, acc);
            ops->cleanup(head.type, size, buffer, readStart, readEnd, data);
        }
        else if (alignedSize == 0 && ops->cleanup)
        {
            ops->cleanup(head.type, 0, NULL, invalid_nvm_addr, invalid_nvm_addr, data);
        }
        addr = ContinousSpaceCalculateNextAddr(addr, alignedSize + sizeof(struct LogEntryHead), acc);
        ContinousSpaceRead(&head, sizeof(struct LogEntryFoot), addr, acc);
    }

    if (lastReadAddr)
        *lastReadAddr = addr;
}

void LogForEachEntry(struct Log * log, struct LogCleanupOps * opsArray, void * data, struct NVMAccesser * acc)
{
    LogForEachEntryImpl(log, opsArray, data, acc, NULL);
}

int LogStepWriteBegin(struct Log * log, struct LogStepWriteHandle * handle, UINT8 type, UINT32 totalSize,
                      struct PagePool * pool, struct NVMAccesser * acc)
{
    UINT64 alignedSize;
    struct LogEntryFoot foot = {.data = 0};
    int err;

    alignedSize = AlignUp(totalSize, 8);
    handle->head.type = type;
    handle->head.size = totalSize;
    err = ContinousSpaceEssureEnoughSpace(
        &log->cs, log->writeStart, sizeof(struct LogEntryHead) + sizeof(struct LogEntryFoot) + alignedSize, pool, acc);
    ContinousSpaceNotifyUsedSpace(&log->cs, sizeof(struct LogEntryFoot) + alignedSize);
    if (err)
        return err;
    handle->headAddr = log->writeStart;
    handle->entryAddr = ContinousSpaceCalculateNextAddr(handle->headAddr, sizeof(struct LogEntryHead), acc);
    handle->footAddr = ContinousSpaceCalculateNextAddr(handle->entryAddr, alignedSize, acc);

    ContinousSpaceWrite(&foot, sizeof(struct LogEntryFoot), handle->footAddr, acc);
    log->writeStart = handle->footAddr;
    return 0;
}

void LogStepWriteContinue(struct LogStepWriteHandle * handle, void * buffer, UINT64 size, logic_addr_t * entryStartAddr,
                          struct NVMAccesser * acc)
{
    if (entryStartAddr)
        *entryStartAddr = handle->entryAddr;
    ContinousSpaceWrite(buffer, size, handle->entryAddr, acc);
    handle->entryAddr = ContinousSpaceCalculateNextAddr(handle->entryAddr, size, acc);
}

void LogStepWriteEnd(struct LogStepWriteHandle * handle, struct NVMAccesser * acc)
{
    ContinousSpaceWrite(&handle->head, sizeof(struct LogEntryHead), handle->headAddr, acc);
}

void LogRecoveryInit(struct Log * log, logic_addr_t addr)
{
    ContinousSpaceRecoveryInit(&log->cs, addr);
}

void LogRecoveryReadReserveData(logic_addr_t firstAreaAddr, void * buffer, UINT64 size, UINT64 offsetInsideReserveData,
                                struct NVMAccesser * acc)
{
    logic_addr_t startAddr;

    startAddr =
        ContinousSpaceCalculateNextAddr(ContinousSpaceStartCalculate(firstAreaAddr), offsetInsideReserveData, acc);
    ContinousSpaceRead(buffer, size, startAddr, acc);
}

void LogRecovery(struct Log * log, struct FsConstructor * ctor, struct LogCleanupOps * opsArray, void * data,
                 struct NVMAccesser * acc)
{
    logic_addr_t lastReadAddr;

    LogForEachEntryImpl(log, opsArray, data, acc, &lastReadAddr);
    ContinousSpaceRecovery(&log->cs, lastReadAddr, ctor, acc);
}

void LogRecoveryUninit(struct Log * log)
{
    ContinousSpaceRecoveryUninit(&log->cs);
}

void LogRebuildBegin(struct Log * log, logic_addr_t addr, UINT64 reserveSize)
{
    log->reserveSize = AlignUp(reserveSize, 8);
    ContinousSpaceRebuildBegin(&log->cs, addr);
}

void LogRebuild(struct Log * log, struct LogCleanupOps * opsArray, void * data, struct NVMAccesser * acc)
{
    logic_addr_t lastReadAddr;

    LogForEachEntryImpl(log, opsArray, data, acc, &lastReadAddr);
    log->writeStart = lastReadAddr;
}

void LogRebuildReadReserveData(logic_addr_t firstAreaAddr, void * buffer, UINT64 size, UINT64 offsetInsideReserveData,
                               struct NVMAccesser * acc)
{
    LogRecoveryReadReserveData(firstAreaAddr, buffer, size, offsetInsideReserveData, acc);
}

void LogRebuildEnd(struct Log * log, struct NVMAccesser * acc)
{
    ContinousSpaceRebuildEnd(&log->cs, log->writeStart, acc);
}

void LogPrintInfo(struct Log * log)
{
    DEBUG_PRINT("log reserveSize is 0x%lx, writeStart is 0x%lx, areaNum is %ld, restSize is %ld", log->reserveSize,
                log->writeStart, log->cs.areaNum, log->cs.restSize);
}