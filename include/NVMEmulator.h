#ifndef NVM_EMULATOR_H
#define NVM_EMULATOR_H

#include "AtomicFunctions.h"
#include "Types.h"
#include <linux/string.h>

extern void * NVMEmulatorAddr;
extern UINT64 NVMEmulatorSize;

int NVMEmulatorInit(UINT64 size);
void NVMEmulatorUninit(void);

#define NVMInitIMPL(size) NVMEmulatorInit(size)

#define NVMUninitIMPL() NVMEmulatorUninit()

#define NVMReadIMPL(offset, size, buffer)                                                                              \
    ({                                                                                                                 \
        memcpy(buffer, NVMEmulatorAddr + offset, size);                                                                \
        size;                                                                                                          \
    })

#define NVMWriteIMPL(offset, size, buffer)                                                                             \
    ({                                                                                                                 \
        memcpy(NVMEmulatorAddr + offset, buffer, size);                                                                \
        size;                                                                                                          \
    })

#define NVMemcpyIMPL(dst, src, size)                                                                                   \
    ({                                                                                                                 \
        memcpy(NVMEmulatorAddr + dst, NVMEmulatorAddr + src, size);                                                    \
        size;                                                                                                          \
    })

#define NVMemsetIMPL(dst, val, size) ({ memset(NVMEmulatorAddr + dst, val, size); })

#define NVMCASIMPL(offset, oldvalue, newvalue)                                                                         \
    ({                                                                                                                 \
        BOOL ret = BOOL_COMPARE_AND_SWAP((typeof(oldvalue) *)(NVMEmulatorAddr + offset), oldvalue, newvalue);          \
        ret;                                                                                                           \
    })

#define NVMFAAIMPL(offset, delta, type)                                                                                \
    ({                                                                                                                 \
        typeof(type) ret = FETCH_AND_ADD((typeof(type) *)(NVMEmulatorAddr + offset), delta);                           \
        ret;                                                                                                           \
    })

#define NVMFlushIMPL(ptr)
#define NVMLoadBarrierIMPL
#define NVMStoreBarrierIMPL
#define NVMBarrierIMPL()

#define NVMCrashSimulateIMPL()
#define NVMCrashRecoverySimulateIMPL()

#endif