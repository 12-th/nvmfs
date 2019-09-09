#ifndef NVM_PTR_DEFINE_H
#define NVM_PTR_DEFINE_H

#include "Types.h"
#include <linux/typecheck.h>

#define NVM_PTR_DEFINE(name, associateType)                                                                            \
    typedef struct name                                                                                                \
    {                                                                                                                  \
        union {                                                                                                        \
            UINT64 addr;                                                                                               \
            associateType * typeForCheck;                                                                              \
        };                                                                                                             \
    } name##_t

NVM_PTR_DEFINE(nvm_addr, void);

#endif