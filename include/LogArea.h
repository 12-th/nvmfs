#ifndef LOG_AREA_H
#define LOG_AREA_H

#include "Types.h"

typedef UINT8 log_type_t;

struct NVMLogAreaMetadata
{
    nvm_addr_t addr;
    UINT32 size;
};

struct LogArea
{
    nvm_addr_t metadataAddr;
    nvm_addr_t logAreaAddr;
    nvm_addr_t tail;
    UINT32 size;
};

struct LogAreaCleanupOps
{
    void * (*prepare)(UINT8 size, void * data);
    void (*cleanup)(UINT8 size, void * buffer, log_type_t type, void * data);
    void (*cleanupEnd)(void * data);
    void * data;
};

void LogAreaInit(struct LogArea * pArea, nvm_addr_t metadataAddr);
void LogAreaFormat(struct LogArea * pArea, nvm_addr_t metadataAddr, nvm_addr_t logAreaAddr, UINT32 size);
void LogAreaUninit(struct LogArea * pArea);
void LogAreaMetadataSerialize(struct LogArea * pArea);
void LogAreaWrite(struct LogArea * pArea, void * buffer, UINT32 size, log_type_t type);
void LogAreaCleanupOpsRegister(struct LogAreaCleanupOps * ops, log_type_t type);
void LogAreaClean(struct LogArea * pArea);

#endif