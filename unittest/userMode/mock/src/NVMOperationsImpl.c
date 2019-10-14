#include "Align.h"
#include "Types.h"
#include <stdlib.h>

void * unittestNVM;
UINT64 unittestNVMSize;

int UnittestNVMInit(UINT64 size)
{
    unittestNVM = aligned_alloc(SIZE_2M, AlignUp(size, SIZE_2M));
    unittestNVMSize = AlignUp(size, SIZE_2M);
    return 0;
}

void UnittestNVMUninit(void)
{
    free(unittestNVM);
    unittestNVMSize = 0;
}