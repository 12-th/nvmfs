// #include "NVMAccesser.h"
// #include "NVMOperations.h"
// #include "WearLeveler.h"

// static inline void AssurePhysAddrValid(struct NVMAccesser * acc)
// {
//     if (!acc->flags.physAddrVaildFlag)
//     {
//         acc->physAddr = LogicAddressTranslate(acc->wl, acc->addr);
//     }
// }

// void NVMAccesserInit(struct NVMAccesser * acc, struct WearLeveler * wl, logic_addr_t addr,
//                      enum NVMAccesserRangeSize type)
// {
//     struct NVMAccesserFlags flags = {.pageOrBlock = type, .physAddrVaildFlag = 0};
//     acc->addr = addr;
//     acc->flags = flags;
//     acc->wl = wl;
// }

// void NVMAccesserRead(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer)
// {
//     AssurePhysAddrValid(acc);
//     NVMRead(acc->physAddr + offset, size, buffer);
// }

// void NVMAccesserWrite(struct NVMAccesser * acc, UINT64 offset, UINT64 size, void * buffer)
// {
//     AssurePhysAddrValid(acc);
//     NVMWrite(acc->physAddr + offset, size, buffer);
// #error wearcount increse, notice cocurrent problem
// }