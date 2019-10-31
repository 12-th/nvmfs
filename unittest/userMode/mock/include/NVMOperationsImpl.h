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
        memcpy(buffer, unittestNVM + offset, size);                                                                    \
        size;                                                                                                          \
    })

#define NVMWriteIMPL(offset, size, buffer)                                                                             \
    ({                                                                                                                 \
        memcpy(unittestNVM + offset, buffer, size);                                                                    \
        size;                                                                                                          \
    })

#define NVMemcpyIMPL(dst, src, size)                                                                                   \
    ({                                                                                                                 \
        memcpy(unittestNVM + dst, unittestNVM + src, size);                                                            \
        size;                                                                                                          \
    })

#define NVMemsetIMPL(dst, val, size) ({ memset(unittestNVM + dst, val, size); })

#define NVMCASIMPL(offset, oldvalue, newvalue)                                                                         \
    ({                                                                                                                 \
        BOOL ret = BOOL_COMPARE_AND_SWAP((typeof(oldvalue) *)(unittestNVM + offset), oldvalue, newvalue);              \
        ret;                                                                                                           \
    })

#define NVMFAAIMPL(offset, delta, type)                                                                                \
    ({                                                                                                                 \
        typeof(type) ret = FETCH_AND_ADD((typeof(type) *)(unittestNVM + offset), delta);                               \
        ret;                                                                                                           \
    })

#define NVMFlushIMPL(ptr)
#define NVMLoadBarrierIMPL
#define NVMStoreBarrierIMPL
#define NVMBarrierIMPL()

#define NVMCrashSimulateIMPL()
#define NVMCrashRecoverySimulateIMPL()

#endif