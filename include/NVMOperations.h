#ifndef NVM_OPERATIONS_H
#define NVM_OPERATIONS_H

#include "NVMOperationsImpl.h"
#include "Types.h"

#define ALWAYS_INLINE __attribute__((always_inline))

static inline ALWAYS_INLINE int NVMInit(UINT64 size)
{
    return NVMInitIMPL(size);
}

static inline ALWAYS_INLINE void NVMUninit(void)
{
    return NVMUninitIMPL();
}

static inline ALWAYS_INLINE UINT64 NVMRead(nvm_addr_t offset, UINT64 size, void * buffer)
{
    return NVMReadIMPL(offset, size, buffer);
}

static inline ALWAYS_INLINE UINT8 NVMRead8(nvm_addr_t offset)
{
    UINT8 value;
    NVMReadIMPL(offset, sizeof(value), &value);
    return value;
}

static inline ALWAYS_INLINE UINT16 NVMRead16(nvm_addr_t offset)
{
    UINT16 value;
    NVMReadIMPL(offset, sizeof(value), &value);
    return value;
}

static inline ALWAYS_INLINE UINT32 NVMRead32(nvm_addr_t offset)
{
    UINT32 value;
    NVMReadIMPL(offset, sizeof(value), &value);
    return value;
}

static inline ALWAYS_INLINE UINT64 NVMRead64(nvm_addr_t offset)
{
    UINT64 value;
    NVMReadIMPL(offset, sizeof(value), &value);
    return value;
}

static inline ALWAYS_INLINE UINT64 NVMWrite(nvm_addr_t offset, UINT64 size, void * buffer)
{
    return NVMWriteIMPL(offset, size, buffer);
}

static inline ALWAYS_INLINE UINT64 NVMWrite8(nvm_addr_t offset, UINT8 data)
{
    return NVMWrite(offset, sizeof(data), &data);
}

static inline ALWAYS_INLINE UINT64 NVMWrite16(nvm_addr_t offset, UINT16 data)
{
    return NVMWrite(offset, sizeof(data), &data);
}

static inline ALWAYS_INLINE UINT64 NVMWrite32(nvm_addr_t offset, UINT32 data)
{
    return NVMWrite(offset, sizeof(data), &data);
}

static inline ALWAYS_INLINE UINT64 NVMWrite64(nvm_addr_t offset, UINT64 data)
{
    return NVMWrite(offset, sizeof(data), &data);
}

static inline ALWAYS_INLINE UINT64 NVMemcpy(nvm_addr_t dst, nvm_addr_t src, UINT64 size)
{
    return NVMemcpyIMPL(dst, src, size);
}

static inline ALWAYS_INLINE void NVMemset(nvm_addr_t dst, int val, UINT64 size)
{
    NVMemsetIMPL(dst, val, size);
}

// BOOL NVMCAS(nvm_addr_t offset, newvalue, oldvalue)
// newvalue && oldvalue could be builtin integer types
#define NVMCAS(offset, oldvalue, newvalue)                                                                             \
    ({                                                                                                                 \
        typecheck(nvm_addr_t, offset);                                                                                 \
        typecheck(typeof(oldvalue), newvalue);                                                                         \
        STATIC_ASSERT((sizeof(oldvalue) <= 8), cannot_cas_with_value_size_large_than_8);                               \
        BOOL ret = NVMCASIMPL(offset, oldvalue, newvalue);                                                             \
        ret;                                                                                                           \
    })

// BOOL NVMFAA(nvm_addr_t offset, type delta)
// return old value
#define NVMFAA(offset, delta, type)                                                                                    \
    ({                                                                                                                 \
        typecheck(nvm_addr_t, offset);                                                                                 \
        STATIC_ASSERT(sizeof(type) <= 8, cannot_faa_with_size_larger_than_8);                                          \
        typeof(type) ret = NVMFAAIMPL(offset, delta);                                                                  \
        ret;                                                                                                           \
    })

#define NVMFlush(ptr, size)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        int i = 0;                                                                                                     \
        for (; i < size; i += 8)                                                                                       \
        {                                                                                                              \
            NVMFlushIMPL(ptr.addr + i);                                                                                \
        }                                                                                                              \
    } while (0)

#define NVMBarrier() NVMBarrierIMPL

#undef ALWAYS_INLINE
#endif