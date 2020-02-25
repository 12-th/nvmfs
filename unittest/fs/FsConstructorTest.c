#include "BlockPool.h"
#include "DirInodeInfo.h"
#include "FileInodeInfo.h"
#include "FsConstructor.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>
#include <linux/string.h>

#define FsConstructorTestVariableDefine()                                                                              \
    struct WearLeveler wl;                                                                                             \
    struct NVMAccesser acc;                                                                                            \
    struct PagePool ppool;                                                                                             \
    struct BlockPool bpool;                                                                                            \
    struct ExtentContainer container;                                                                                  \
    struct NvmInode dirInode;                                                                                          \
    struct NvmInode fileInode;

#define FsConstructorTestVariableInit()                                                                                \
    NVMInit(1UL << 30);                                                                                                \
    WearLevelerFormat(&wl, 30, 0);                                                                                     \
    NVMAccesserInit(&acc, &wl);                                                                                        \
    ExtentContainerInit(&container, GFP_KERNEL);                                                                       \
    ExtentContainerAppend(&container, 0, 1UL << 30, GFP_KERNEL);                                                       \
    BlockPoolInit(&bpool, &acc);                                                                                       \
    BlockPoolExtentPut(&bpool, &container);                                                                            \
    ExtentContainerUninit(&container);                                                                                 \
    PagePoolInit(&ppool, &bpool, acc);

#define FsConstructorTestVariableUninit()                                                                              \
    PagePoolUninit(&ppool);                                                                                            \
    BlockPoolUninit(&bpool);                                                                                           \
    NVMAccesserUninit(&acc);                                                                                           \
    WearLevelerUninit(&wl);                                                                                            \
    NVMUninit();

// TEST(FsConstructorTest, wholeTest)
// {
//     FsConstructorTestVariableDefine();
//     struct FsConstructor ctor;
//     struct DirInodeInfo dirInfo;
//     struct FileInodeInfo fileInfo;
//     struct InodeTable table;

//     FsConstructorTestVariableInit();
//     DirInodeInfoFormat(&dirInfo, &ppool, &dirInode, &acc);
//     FileInodeInfoFormat(&fileInfo, &ppool, &bpool, &fileInode, &acc);
//     InodeTableFormat(&table, 0, )

//     FsConstructorTestVariableUninit();
// }