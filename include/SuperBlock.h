#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H

#include "Types.h"

struct AvailBlockTable;

struct NVMSuperBlock
{
    nvm_addr_t availBlockTable;
    nvm_addr_t blockMetaDataTable;
};

struct SuperBlock
{
    UINT64 nvmSizeBits;
    nvm_addr_t addr;
    struct AvailBlockTable * pAvailBlockTable;
};

static inline UINT64 NvmBitsQuery(struct SuperBlock * sb)
{
    return sb->nvmSizeBits;
}

static inline UINT64 BlockNumQuery(struct SuperBlock * sb)
{
    return 1UL << (sb->nvmSizeBits - BITS_2M);
}

#endif