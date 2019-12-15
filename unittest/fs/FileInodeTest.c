#include "BlockPool.h"
#include "FileInodeInfo.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>
#include <linux/string.h>

#define FileInodeTestVariableDefine()                                                                                  \
    struct WearLeveler wl;                                                                                             \
    struct NVMAccesser acc;                                                                                            \
    struct PagePool ppool;                                                                                             \
    struct BlockPool bpool;                                                                                            \
    struct FileInodeInfo info;                                                                                         \
    struct ExtentContainer container;                                                                                  \
    logic_addr_t firstArea;

#define FileInodeTestVariableInit()                                                                                    \
    NVMInit(1UL << 30);                                                                                                \
    WearLevelerFormat(&wl, 30, 0);                                                                                     \
    NVMAccesserInit(&acc, &wl);                                                                                        \
    ExtentContainerInit(&container, GFP_KERNEL);                                                                       \
    ExtentContainerAppend(&container, 0, 1UL << 30, GFP_KERNEL);                                                       \
    BlockPoolInit(&bpool);                                                                                             \
    BlockPoolExtentPut(&bpool, &container);                                                                            \
    ExtentContainerUninit(&container);                                                                                 \
    PagePoolInit(&ppool, &bpool, acc);

#define FileInodeTestVariableUninit()                                                                                  \
    PagePoolUninit(&ppool);                                                                                            \
    BlockPoolUninit(&bpool);                                                                                           \
    NVMAccesserUninit(&acc);                                                                                           \
    WearLevelerUninit(&wl);                                                                                            \
    NVMUninit();

TEST(FileInodeTest, formatTest1)
{
    FileInodeTestVariableDefine();
    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, formatTest)
{
    FileInodeTestVariableDefine();
    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, readTest1)
{
    FileInodeTestVariableDefine();
    char * buffer;
    UINT64 i;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    buffer = __get_free_page(GFP_KERNEL);
    FileInodeInfoReadData(&info, buffer, PAGE_SIZE, 0, &acc);

    for (i = 0; i < 4096; ++i)
    {
        EXPECT_EQ(buffer[i], (char)0);
    }

    free_page((unsigned long)buffer);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, readTest2)
{
    FileInodeTestVariableDefine();
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    buffer = __get_free_page(GFP_KERNEL);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }
    // memset(paddingData, 0xabcdef00, PAGE_SIZE);

    FileInodeInfoWriteData(&info, paddingData, PAGE_SIZE, 0, &acc);
    FileInodeInfoReadData(&info, buffer, PAGE_SIZE, 0, &acc);

    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer[i], MAGIC_NUMBER);
    }

    free_page((unsigned long)paddingData);
    free_page((unsigned long)buffer);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}

static inline void ExpectBufferIs(UINT32 * buffer, UINT32 start, UINT32 end, UINT32 value)
{
    UINT32 i;
    for (i = start / sizeof(UINT32); i < end / sizeof(UINT32); ++i)
    {
        EXPECT_EQ(buffer[i], value);
    }
}

TEST(FileInodeTest, readTest3)
{
    FileInodeTestVariableDefine();
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 1);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }
    // memset(paddingData, 0xabcdef00, PAGE_SIZE);

    // 0-512
    FileInodeInfoWriteData(&info, paddingData, 512, 0, &acc);
    // 1024-2048
    FileInodeInfoWriteData(&info, paddingData, 1024, 1024, &acc);
    // 3072-5120
    FileInodeInfoWriteData(&info, paddingData, 2048, 3072, &acc);
    FileInodeInfoReadData(&info, buffer, PAGE_SIZE * 2, 0, &acc);

    ExpectBufferIs(buffer, 0, 512, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 512, 1024, 0);
    ExpectBufferIs(buffer, 1024, 2048, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 2048, 3072, 0);
    ExpectBufferIs(buffer, 3072, 5120, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 5120, 8192, 0);

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, readTest4)
{
    FileInodeTestVariableDefine();
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 1);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }
    // memset(paddingData, 0xabcdef00, PAGE_SIZE);

    // 0-512
    FileInodeInfoWriteData(&info, paddingData, 512, 0, &acc);
    // 1024-2048
    FileInodeInfoWriteData(&info, paddingData, 1024, 1024, &acc);
    // 3072-8192
    FileInodeInfoWriteData(&info, paddingData, 4096, 3072, &acc);
    FileInodeInfoWriteData(&info, paddingData, 1024, 7168, &acc);
    FileInodeInfoReadData(&info, buffer, PAGE_SIZE * 2, 0, &acc);

    ExpectBufferIs(buffer, 0, 512, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 512, 1024, 0);
    ExpectBufferIs(buffer, 1024, 2048, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 2048, 3072, 0);
    ExpectBufferIs(buffer, 3072, 8192, MAGIC_NUMBER);

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, rebuildTest)
{
    FileInodeTestVariableDefine();
    struct FileInodeInfo rebuildInfo;
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;
    logic_addr_t firstAreaAddr;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    firstAreaAddr = LogFirstArea(&info.log);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 1);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }

    // 0-512
    FileInodeInfoWriteData(&info, paddingData, 512, 0, &acc);
    // 1024-2048
    FileInodeInfoWriteData(&info, paddingData, 1024, 1024, &acc);
    // 3072-5120
    FileInodeInfoWriteData(&info, paddingData, 2048, 3072, &acc);

    FileInodeInfoRebuild(&rebuildInfo, firstAreaAddr, &ppool, &bpool, &acc);
    FileInodeInfoReadData(&rebuildInfo, buffer, PAGE_SIZE * 2, 0, &acc);

    ExpectBufferIs(buffer, 0, 512, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 512, 1024, 0);
    ExpectBufferIs(buffer, 1024, 2048, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 2048, 3072, 0);
    ExpectBufferIs(buffer, 3072, 5120, MAGIC_NUMBER);
    ExpectBufferIs(buffer, 5120, 8192, 0);
    EXPECT_TRUE(FileInodeIsInfoSame(&info, &rebuildInfo));

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeInfoUninit(&rebuildInfo);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, rebuildTest2)
{
    FileInodeTestVariableDefine();
    struct FileInodeInfo rebuildInfo;
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;
    logic_addr_t firstAreaAddr;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    firstAreaAddr = LogFirstArea(&info.log);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 1);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }

    FileInodeInfoRebuild(&rebuildInfo, firstAreaAddr, &ppool, &bpool, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 5, 0, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 15, 0, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 10, 0, &acc);
    FileInodeInfoReadData(&rebuildInfo, buffer, 15, 0, &acc);
    ExpectBufferIs(buffer, 0, 15, MAGIC_NUMBER);

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeInfoUninit(&rebuildInfo);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, rebuildTest3)
{
    FileInodeTestVariableDefine();
    struct FileInodeInfo rebuildInfo;
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;
    logic_addr_t firstAreaAddr;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    firstAreaAddr = LogFirstArea(&info.log);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 1);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }

    FileInodeInfoRebuild(&rebuildInfo, firstAreaAddr, &ppool, &bpool, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 0, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096, &acc);
    FileInodeInfoReadData(&rebuildInfo, buffer, 8192, 0, &acc);
    ExpectBufferIs(buffer, 0, 8192, MAGIC_NUMBER);

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeInfoUninit(&rebuildInfo);
    FileInodeTestVariableUninit();
}

TEST(FileInodeTest, rebuildTest4)
{
    FileInodeTestVariableDefine();
    struct FileInodeInfo rebuildInfo;
    UINT32 * buffer;
    UINT32 * paddingData;
    UINT64 i;
    UINT32 MAGIC_NUMBER = 0xabcdef00;
    logic_addr_t firstAreaAddr;

    FileInodeTestVariableInit();
    FileInodeInfoFormat(&info, &ppool, &bpool, &firstArea, &acc);
    firstAreaAddr = LogFirstArea(&info.log);
    buffer = (UINT32 *)__get_free_pages(GFP_KERNEL, 3);
    paddingData = __get_free_page(GFP_KERNEL);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT32); ++i)
    {
        paddingData[i] = MAGIC_NUMBER;
    }

    FileInodeInfoRebuild(&rebuildInfo, firstAreaAddr, &ppool, &bpool, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 0, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096, &acc);
    FileInodeInfoReadData(&rebuildInfo, buffer, 8192, 0, &acc);
    ExpectBufferIs(buffer, 0, 8192, MAGIC_NUMBER);
    FileInodeInfoUninit(&rebuildInfo);

    FileInodeInfoRebuild(&rebuildInfo, firstAreaAddr, &ppool, &bpool, &acc);
    FileInodePrintInfo(&rebuildInfo);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 0, &acc);
    FileInodePrintInfo(&rebuildInfo);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096, &acc);
    FileInodePrintInfo(&rebuildInfo);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096 * 2, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096 * 3, &acc);
    FileInodeInfoWriteData(&rebuildInfo, paddingData, 4096, 4096 * 4, &acc);
    FileInodeInfoReadData(&rebuildInfo, buffer, 4096 * 5, 0, &acc);
    ExpectBufferIs(buffer, 0, 4096 * 5, MAGIC_NUMBER);

    free_page((unsigned long)paddingData);
    free_pages((unsigned long)buffer, 1);
    FileInodeInfoUninit(&info);
    FileInodeTestVariableUninit();
}
