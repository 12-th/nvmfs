#include "NVMEmulator.h"
#include "Types.h"
#include "common.h"
#include <linux/mm.h>

void * NVMEmulatorAddr;
UINT64 NVMEmulatorSize;

int NVMEmulatorInit(UINT64 size)
{
    NVMEmulatorAddr = vmalloc(size);
    DEBUG_PRINT("nvm vmalloc emulator init, start addr is 0x%lx", (unsigned long)NVMEmulatorAddr);
    return 0;
}

void NVMEmulatorUninit(void)
{
    DEBUG_PRINT("nvm vmalloc emulator uninit");
    vfree(NVMEmulatorAddr);
}