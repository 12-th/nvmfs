#include "BlockPool.h"
#include "DirInode.h"
#include "Inode.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>
#include <linux/string.h>

#define DirInodeTestVariableDefine()                                                                                   \
    struct WearLeveler wl;                                                                                             \
    struct NVMAccesser acc;                                                                                            \
    struct PagePool ppool;                                                                                             \
    struct BlockPool bpool;                                                                                            \
    struct DirInodeInfo info;                                                                                          \
    struct ExtentContainer container;                                                                                  \
    struct NvmInode inode;

#define DirInodeTestVariableInit()                                                                                     \
    NVMInit(1UL << 30);                                                                                                \
    WearLevelerFormat(&wl, 30, 0);                                                                                     \
    NVMAccesserInit(&acc, &wl);                                                                                        \
    ExtentContainerInit(&container, GFP_KERNEL);                                                                       \
    ExtentContainerAppend(&container, 0, 1UL << 30, GFP_KERNEL);                                                       \
    BlockPoolInit(&bpool);                                                                                             \
    BlockPoolExtentPut(&bpool, &container);                                                                            \
    ExtentContainerUninit(&container);                                                                                 \
    PagePoolInit(&ppool, &bpool, acc);

#define DirInodeTestVariableUninit()                                                                                   \
    PagePoolUninit(&ppool);                                                                                            \
    BlockPoolUninit(&bpool);                                                                                           \
    NVMAccesserUninit(&acc);                                                                                           \
    WearLevelerUninit(&wl);                                                                                            \
    NVMUninit();

TEST(DirInodeInfoTest, FormatTest)
{
    DirInodeTestVariableDefine();
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &inode, &acc);
    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

#define TEST_NAME1 "hello"
#define TEST_NAME1_SIZE 5
#define TEST_NAME2 "world"
#define TEST_NAME2_SIZE 5

TEST(DirInodeInfoTest, AddRemoveDentryTest1)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &inode, &acc);

    DirInodeAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_EQ(err, 0);
    DirInodeRemoveDentry(&info, 1, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, AddRemoveDentryTest2)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &inode, &acc);

    DirInodeAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_EQ(err, 0);

    DirInodeAddDentry(&info, 2, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    err = DirInodeInfoLookupDentry(&info, 2);
    EXPECT_EQ(err, 0);

    DirInodeRemoveDentry(&info, 1, &acc);
    DirInodeRemoveDentry(&info, 2, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_NE(err, 0);
    err = DirInodeInfoLookupDentry(&info, 2);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, AddRemoveDentryTest3)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &inode, &acc);

    DirInodeAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_EQ(err, 0);

    err = DirInodeAddDentry(&info, 1, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    EXPECT_NE(err, 0);

    DirInodeRemoveDentry(&info, 1, &acc);
    err = DirInodeInfoLookupDentry(&info, 1);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}