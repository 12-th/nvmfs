#ifndef NVM_OPERATIONS_IMPL_H
#define NVM_OPERATIONS_IMPL_H

#include "AtomicFunctions.h"
#include "Types.h"
#include <string.h>

extern void * unittestNVM;
extern UINT64 unittestNVMSize;
extern int UnittestNVMInit(UINT64 size);
extern void UnittestNVMUninit(void);

#define NVMInitIMPL(size) UnittestNVMInit(size)

#define NVMUninitIMPL() UnittestNVMUninit()

#define NVMReadIMPL(offset, size, buffer)                                                                              \
    ({                                                                                                                 \
        memcpy(buffer, unittestNVM + offset.addr, size);                                                               \
        size;                                                                                                          \
    })

#define NVMWriteIMPL(offset, size, buffer)                                                                             \
    ({                                                                                                                 \
        memcpy(unittestNVM + offset.addr, buffer, size);                                                               \
        size;                                                                                                          \
    })

#define NVMemcpyIMPL(dst, src, size)                                                                                   \
    ({                                                                                                                 \
        memcpy(unittestNVM + dst.addr, unittestNVM + src.addr, size);                                                  \
        size;                                                                                                          \
    })

#define NVMemsetIMPL(dst, val, size) ({ memset(unittestNVM + dst.addr, val, size); })

#define NVMCASIMPL(offset, oldvalue, newvalue)                                                                         \
    ({                                                                                                                 \
        BOOL ret = BOOL_COMPARE_AND_SWAP((typeof(oldvalue) *)(unittestNVM + offset.addr), oldvalue, newvalue);         \
        ret;                                                                                                           \
    })

#define NVMFAAIMPL(offset, delta, type)                                                                                \
    ({                                                                                                                 \
        typeof(type) ret = FETCH_AND_ADD((typeof(type) *)(unittestNVM + offset.addr), delta);                          \
        ret;                                                                                                           \
    })

#define NVMFlushIMPL
#define NVMLoadBarrierIMPL
#define NVMStoreBarrierIMPL

#endif