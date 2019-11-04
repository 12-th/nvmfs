#include "NVMOperations.h"
#include "SuperBlock.h"

void SuperBlockFormat(struct SuperBlock * sb, UINT32 nvmSizeBits)
{
    struct NVMSuperBlock data = {.nvmSizeBits = nvmSizeBits, .shutdownFlag = SUPER_BLOCK_SHUTDOWN_FLAG_NORMAL};
    NVMWrite(0, sizeof(struct NVMSuperBlock), &data);
    sb->nvmSizeBits = nvmSizeBits;
    sb->shutdownFlag = SUPER_BLOCK_SHUTDOWN_FLAG_NORMAL;
}

void SuperBlockInit(struct SuperBlock * sb)
{
    struct NVMSuperBlock data;
    NVMRead(0, sizeof(struct SuperBlock), &data);

    sb->nvmSizeBits = data.nvmSizeBits;
    sb->shutdownFlag = data.shutdownFlag;
}

void SetSuperBlockShutdownFlags(struct SuperBlock * sb)
{
    NVMWrite64((UINT64)(&(((struct NVMSuperBlock *)0)->shutdownFlag)), sb->shutdownFlag);
}
