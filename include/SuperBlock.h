#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H

#include "Types.h"
#include "common.h"

#define SUPER_BLOCK_SHUTDOWN_FLAG_NORMAL 1
#define SUPER_BLOCK_SHUTDWON_FLAG_ERROR 2

struct NVMSuperBlock
{
    UINT64 nvmSizeBits;
    UINT64 shutdownFlag;
} __attribute__((packed));

struct SuperBlock
{
    UINT64 nvmSizeBits;
    UINT64 shutdownFlag;
};

void SuperBlockFormat(struct SuperBlock * sb, UINT32 nvmSizeBits);
void SuperBlockInit(struct SuperBlock * sb);
void SetSuperBlockShutdownFlags(struct SuperBlock * sb);

static inline ALWAYS_INLINE UINT64 NvmBitsQuery(struct SuperBlock * sb)
{
    return sb->nvmSizeBits;
}

static inline ALWAYS_INLINE UINT64 ShutdownFlagQuery(struct SuperBlock * sb)
{
    return sb->shutdownFlag;
}

#endif