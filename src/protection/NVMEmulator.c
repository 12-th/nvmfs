#include "NVMEmulator.h"
#include "Types.h"
#include <linux/mm.h>

void * NVMEmulatorAddr;
UINT64 NVMEmulatorSize;

int NVMEmulatorInit(UINT64 size)
{
    NVMEmulatorAddr = vmalloc(size);
    return 0;
}

void NVMEmulatorUninit(void)
{
    vfree(NVMEmulatorAddr);
}