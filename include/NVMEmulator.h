#ifndef NVM_EMULATOR_H
#define NVM_EMULATOR_H

#error to be implement

#define NVMReadIMPL(offset, size, buffer)

#define NVMWriteIMPL(offset, size, buffer)

#define NVMemcpyIMPL(dst, src, size)

#define NVMemsetIMPL(dst, val, size)

#define NVMInitIMPL(size)

#define NVMUninitIMPL()

#define NVMCASIMPL(offset, size, oldvalue, newvalue)

#define NVMFAAIMPL(offset, size, oldvalue, newvalue)

#define NVMFlushIMPL
#define NVMBarrierIMPL

#endif