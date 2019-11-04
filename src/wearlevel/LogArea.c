#include "Config.h"
#include "LogArea.h"
#include "NVMOperations.h"
#include "common.h"
#include <Align.h>
#include <linux/string.h>

struct LogEntryHead
{
    log_type_t type;
    UINT8 size;
} __attribute__((packed));

struct LogEntryFoot
{
    union {
        struct LogEntryHead head;
        UINT16 data;
    };
} __attribute__((packed));

static struct LogAreaCleanupOps * cleanupOps[LOG_TYPE_MAX_NUM];

static inline UINT32 LogAquireSize(UINT32 dataSize)
{
    return dataSize + sizeof(struct LogEntryFoot) + sizeof(struct LogEntryHead);
}

static inline void InitLogEntryHead(struct LogEntryHead * head, log_type_t type, UINT8 dataSize)
{
    head->type = type;
    head->size = dataSize;
}

static inline void InitLogentryFoot(struct LogEntryFoot * foot)
{
    memset(foot, 0, sizeof(struct LogEntryFoot));
}

static inline int HasNextLogEntry(struct LogEntryFoot * foot)
{
    return foot->data != 0;
}

void LogAreaClean(struct LogArea * pArea)
{
    nvm_addr_t addr;
    struct LogEntryHead * head;
    struct LogEntryFoot foot;
    log_type_t i;

    head = &(foot.head);
    addr = pArea->logAreaAddr;
    NVMRead(addr, sizeof(struct LogEntryHead), head);
    while (HasNextLogEntry(&foot))
    {
        if (cleanupOps[head->type])
        {
            struct LogAreaCleanupOps * ops;
            void * buffer;

            ops = cleanupOps[head->type];
            buffer = ops->prepare(head->size, ops->data);
            if (buffer)
            {
                NVMRead(addr + sizeof(struct LogEntryHead), head->size, buffer);
                ops->cleanup(head->size, buffer, head->type, ops->data);
            }
        }
        addr = AlignUpBits(addr + sizeof(struct LogEntryHead) + head->size, 3);
        NVMRead(addr, sizeof(struct LogEntryHead), head);
    }

    for (i = 0; i < LOG_TYPE_MAX_NUM; ++i)
    {
        if (cleanupOps[i])
            cleanupOps[i]->cleanupEnd(cleanupOps[i]->data);
    }

    pArea->tail = pArea->logAreaAddr;
}

void LogAreaInit(struct LogArea * pArea, nvm_addr_t metadataAddr)
{
    struct NVMLogAreaMetadata metadata;

    pArea->metadataAddr = metadataAddr;
    NVMRead(metadataAddr, sizeof(struct NVMLogAreaMetadata), &metadata);
    pArea->logAreaAddr = metadata.addr;
    pArea->size = metadata.size;
    pArea->tail = metadata.addr;
}

void LogAreaFormat(struct LogArea * pArea, nvm_addr_t metadataAddr, nvm_addr_t logAreaAddr, UINT32 size)
{
    struct NVMLogAreaMetadata metadata = {.addr = logAreaAddr, .size = size};
    struct LogEntryFoot foot;

    InitLogentryFoot(&foot);

    NVMWrite(metadataAddr, sizeof(struct NVMLogAreaMetadata), &metadata);
    pArea->metadataAddr = metadataAddr;
    pArea->logAreaAddr = metadata.addr;
    pArea->size = metadata.size;
    pArea->tail = metadata.addr;
    NVMWrite(metadata.addr, sizeof(struct LogEntryFoot), &foot);
}

void LogAreaUninit(struct LogArea * pArea)
{
}

void LogAreaMetadataSerialize(struct LogArea * pArea)
{
    struct NVMLogAreaMetadata metadata = {.addr = pArea->metadataAddr, .size = pArea->size};
    NVMWrite(pArea->metadataAddr, sizeof(struct NVMLogAreaMetadata), &metadata);
}

void LogAreaWrite(struct LogArea * pArea, void * buffer, UINT32 size, log_type_t type)
{
    UINT32 restSize;
    struct LogEntryHead head;
    struct LogEntryFoot foot;
    UINT64 headAddr, dataAddr, footAddr;

    ASSERT(size <= (1Ul << (sizeof(UINT8) * 8)));

    InitLogEntryHead(&head, type, size);
    InitLogentryFoot(&foot);
    restSize = pArea->size - (pArea->tail - pArea->logAreaAddr);
    if (restSize < LogAquireSize(size))
    {
        LogAreaClean(pArea);
    }

    headAddr = pArea->tail;
    dataAddr = headAddr + sizeof(struct LogEntryHead);
    footAddr = AlignUpBits(dataAddr + size, 3);
    NVMWrite(dataAddr, size, buffer);
    NVMWrite(footAddr, sizeof(struct LogEntryFoot), &foot);
    NVMFlush(dataAddr, footAddr - dataAddr + sizeof(struct LogEntryFoot));
    NVMBarrier();
    NVMWrite(headAddr, sizeof(struct LogEntryHead), &head);
    NVMFlush(headAddr, sizeof(struct LogEntryHead));
    NVMBarrier();

    pArea->tail = footAddr;
}

void LogAreaCleanupOpsRegister(struct LogAreaCleanupOps * ops, log_type_t type)
{
    cleanupOps[type] = ops;
}