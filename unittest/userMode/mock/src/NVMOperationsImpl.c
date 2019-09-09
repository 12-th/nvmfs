#include "Types.h"
#include <stdlib.h>

void * unittestNVM;
UINT64 unittestNVMSize;

int UnittestNVMInit(UINT64 size)
{
    unittestNVM = malloc(size);
    unittestNVMSize = size;
    return 0;
}

void UnittestNVMUninit(void)
{
    free(unittestNVM);
    unittestNVMSize = 0;
}