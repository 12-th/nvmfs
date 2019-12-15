#include "BlockPool.h"
#include "DirInodeInfo.h"
#include "Inode.h"
#include "InodeType.h"
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
    logic_addr_t firstArea;

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
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);
    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

#define TEST_NAME1 "hello"
#define TEST_NAME1_SIZE 6
#define TEST_NAME2 "world"
#define TEST_NAME2_SIZE 6

TEST(DirInodeInfoTest, AddRemoveDentryTest1)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);

    DirInodeInfoAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_EQ(err, 0);
    DirInodeInfoRemoveDentry(&info, 1, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, AddRemoveDentryTest2)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);

    DirInodeInfoAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_EQ(err, 0);

    DirInodeInfoAddDentry(&info, 2, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 2);
    EXPECT_EQ(err, 0);

    DirInodeInfoRemoveDentry(&info, 1, &acc);
    DirInodeInfoRemoveDentry(&info, 2, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_NE(err, 0);
    err = DirInodeInfoLookupDentryByIno(&info, 2);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, AddRemoveDentryTest3)
{
    DirInodeTestVariableDefine();
    int err;
    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);

    DirInodeInfoAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_EQ(err, 0);

    err = DirInodeInfoAddDentry(&info, 1, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    EXPECT_NE(err, 0);

    DirInodeInfoRemoveDentry(&info, 1, &acc);
    err = DirInodeInfoLookupDentryByIno(&info, 1);
    EXPECT_NE(err, 0);

    DirInodeInfoUninit(&info);
    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, RebuildTest1)
{
    DirInodeTestVariableDefine();
    struct DirInodeInfo rebuildInfo;
    logic_addr_t addr;
    int err;

    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);
    DirInodeInfoAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    DirInodeInfoAddDentry(&info, 2, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    DirInodeInfoRemoveDentry(&info, 1, &acc);
    addr = LogFirstArea(&info.log);
    DirInodeInfoUninit(&info);

    DirInodeInfoRebuild(&rebuildInfo, addr, &ppool, &acc);
    EXPECT_EQ(rebuildInfo.cache.dentryNum, info.cache.dentryNum);
    EXPECT_EQ(rebuildInfo.cache.totalDentryNameLen, info.cache.totalDentryNameLen);
    EXPECT_EQ(rebuildInfo.pool, info.pool);
    EXPECT_TRUE(LogIsSame(&rebuildInfo.log, &info.log));
    err = DirInodeInfoLookupDentryByIno(&rebuildInfo, 1);
    EXPECT_NE(err, 0);
    err = DirInodeInfoLookupDentryByIno(&rebuildInfo, 2);
    EXPECT_EQ(err, 0);
    DirInodeInfoUninit(&rebuildInfo);

    DirInodeTestVariableUninit();
}

TEST(DirInodeInfoTest, RebuildTest3)
{
    DirInodeTestVariableDefine();
    struct DirInodeInfo rebuildInfo;
    logic_addr_t addr;
    int err;
    UINT32 *name, *retName;
    int i;
    UINT32 MAGIC_NUMBER = 0xabcdefab;

    DirInodeTestVariableInit();
    DirInodeInfoFormat(&info, &ppool, &firstArea, &acc);
    name = kmalloc(PAGE_SIZE, GFP_KERNEL);
    retName = kmalloc(PAGE_SIZE, GFP_KERNEL);
    for (i = 0; i < (PAGE_SIZE / sizeof(UINT32) - 1); ++i)
    {
        name[i] = MAGIC_NUMBER;
    }
    name[i] = 0;

    DirInodeInfoAddDentry(&info, 1, TEST_NAME1, TEST_NAME1_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    DirInodeInfoAddDentry(&info, 2, TEST_NAME2, TEST_NAME2_SIZE, INODE_TYPE_DIR_FILE, &acc);
    DirInodeInfoAddDentry(&info, 3, (char *)name, PAGE_SIZE, INODE_TYPE_REGULAR_FILE, &acc);
    DirInodeInfoRemoveDentry(&info, 1, &acc);
    addr = LogFirstArea(&info.log);
    DirInodeInfoUninit(&info);

    DirInodeInfoRebuild(&rebuildInfo, addr, &ppool, &acc);
    EXPECT_EQ(rebuildInfo.cache.dentryNum, info.cache.dentryNum);
    EXPECT_EQ(rebuildInfo.cache.totalDentryNameLen, info.cache.totalDentryNameLen);
    EXPECT_EQ(rebuildInfo.pool, info.pool);
    EXPECT_TRUE(LogIsSame(&rebuildInfo.log, &info.log));
    err = DirInodeInfoLookupAndGetDentryName(&rebuildInfo, 1, (char *)retName, PAGE_SIZE, &acc);
    EXPECT_NE(err, 0);
    err = DirInodeInfoLookupAndGetDentryName(&rebuildInfo, 2, (char *)retName, PAGE_SIZE, &acc);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(strcmp(TEST_NAME2, (char *)retName), 0);
    err = DirInodeInfoLookupAndGetDentryName(&rebuildInfo, 3, (char *)retName, PAGE_SIZE, &acc);
    EXPECT_EQ(err, 0);
    for (i = 0; i < (PAGE_SIZE / sizeof(UINT32) - 1); ++i)
    {
        EXPECT_EQ(name[i], retName[i]);
    }
    DirInodeInfoUninit(&rebuildInfo);

    kfree(retName);
    kfree(name);
    DirInodeTestVariableUninit();
}