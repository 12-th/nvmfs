#ifndef LOG_AREA_H
#define LOG_AREA_H

#include "ContinousSpace.h"
#include "PagePool.h"
#include "Types.h"
#include <linux/mutex.h>

typedef UINT8 log_type_t;

struct FsConstructor;
struct PagePool;
struct NVMAccesser;

struct LogEntryHead
{
    union {
        struct
        {
            log_type_t type;
            UINT32 size;
        };
        UINT64 data;
    };
};

struct LogCleanupOps
{
    void * (*prepare)(UINT32 * size, void * data);
    void (*cleanup)(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                    logic_addr_t entryReadEndAddr, void * data);
};

struct Log
{
    UINT64 reserveSize;
    logic_addr_t writeStart;
    struct ContinousSpace cs;
};

struct LogStepWriteHandle
{
    logic_addr_t headAddr;
    logic_addr_t entryAddr;
    logic_addr_t footAddr;
    struct LogEntryHead head;
};

int LogFormat(struct Log * log, struct PagePool * pool, UINT64 reserveSize, struct NVMAccesser * acc);
void LogUninit(struct Log * log);
void LogDestroy(struct Log * log, struct NVMAccesser * acc, struct PagePool * pool);
int LogWrite(struct Log * log, UINT64 size, void * buffer, UINT8 type, logic_addr_t * entryStartAddr,
             struct PagePool * pool, struct NVMAccesser * acc);
void LogWriteReserveData(struct Log * log, UINT64 size, UINT64 offsetInsideReserveData, void * buffer,
                         struct NVMAccesser * acc);
void LogRead(struct Log * log, logic_addr_t addr, UINT64 size, void * buffer, struct NVMAccesser * acc);
void LogForEachEntry(struct Log * log, struct LogCleanupOps * opsArray, void * data, struct NVMAccesser * acc);
int LogStepWriteBegin(struct Log * log, struct LogStepWriteHandle * handle, UINT8 type, UINT32 totalSize,
                      struct PagePool * pool, struct NVMAccesser * acc);
void LogStepWriteContinue(struct LogStepWriteHandle * handle, void * buffer, UINT64 size, logic_addr_t * entryStartAddr,
                          struct NVMAccesser * acc);
void LogStepWriteEnd(struct LogStepWriteHandle * handle, struct NVMAccesser * acc);

static inline logic_addr_t LogFirstArea(struct Log * log)
{
    return ContinousSpaceFirstArea(&log->cs);
}

static inline UINT64 LogTotalSizeQuery(struct Log * log)
{
    return ContinousSpaceTotalSizeQuery(&log->cs);
}

void LogRecoveryInit(struct Log * log, logic_addr_t addr);
void LogRecovery(struct Log * log, struct FsConstructor * ctor, struct LogCleanupOps * opsArray, void * data,
                 struct NVMAccesser * acc);
void LogRecoveryReadReserveData(logic_addr_t firstAreaAddr, void * buffer, UINT64 size, UINT64 offsetInsideReserveData,
                                struct NVMAccesser * acc);
void LogRecoveryUninit(struct Log * log);

void LogRebuildBegin(struct Log * log, logic_addr_t addr, UINT64 reserveSize);
void LogRebuild(struct Log * log, struct LogCleanupOps * opsArray, void * data, struct NVMAccesser * acc);
void LogRebuildReadReserveData(logic_addr_t firstAreaAddr, void * buffer, UINT64 size, UINT64 offsetInsideReserveData,
                               struct NVMAccesser * acc);
void LogRebuildEnd(struct Log * log, struct NVMAccesser * acc);

#endif