#include "BlockPool.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>

TEST(PagePoolTest, AllocFreeTest1)
{
    struct BlockPool bpool;
    struct PagePool ppool;
    logical_page_t page;
    struct ExtentContainer container;
    struct WearLeveler wl;
    struct NVMAccesser acc;

    NVMInit(1UL << 30);
    WearLevelerFormat(&wl, 30, 0);
    NVMAccesserInit(&acc, &wl);

    PagePoolGlobalInit();
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    page = PagePoolAlloc(&ppool);
    EXPECT_EQ(page, 0);
    PagePoolFree(&ppool, page);
    EXPECT_EQ(ppool.subPoolNum, 1);

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
    PagePoolGlobalUninit();

    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(PagePoolTest, AllocFreeTest2)
{
    struct BlockPool bpool;
    struct PagePool ppool;
    logical_page_t * pages;
    struct ExtentContainer container;
    UINT64 i;

    struct WearLeveler wl;
    struct NVMAccesser acc;

    NVMInit(1UL << 30);
    WearLevelerFormat(&wl, 30, 0);
    NVMAccesserInit(&acc, &wl);

    pages = kmalloc(sizeof(logical_page_t) * 512, GFP_KERNEL);
    PagePoolGlobalInit();
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    for (i = 0; i < 512; ++i)
    {
        pages[i] = PagePoolAlloc(&ppool);
    }

    for (i = 0; i < 511; i++)
    {
        EXPECT_TRUE(pages[i] + 1 == pages[i + 1]);
    }

    for (i = 0; i < 512; ++i)
    {
        PagePoolFree(&ppool, pages[i]);
    }

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
    PagePoolGlobalUninit();
    kfree(pages);

    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(PagePoolTest, AllocFreeTest3)
{
    struct BlockPool bpool;
    struct PagePool ppool;
    logical_page_t * pages;
    struct ExtentContainer container;
    UINT64 i;

    struct WearLeveler wl;
    struct NVMAccesser acc;

    NVMInit(1UL << 30);
    WearLevelerFormat(&wl, 30, 0);
    NVMAccesserInit(&acc, &wl);

    pages = kmalloc(sizeof(logical_page_t) * 513, GFP_KERNEL);
    PagePoolGlobalInit();
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    for (i = 0; i < 513; ++i)
    {
        pages[i] = PagePoolAlloc(&ppool);
    }

    for (i = 0; i < 511; i++)
    {
        EXPECT_EQ(logical_page_to_block(pages[i]), logical_page_to_block(pages[i + 1]));
    }

    EXPECT_NE(logical_page_to_block(pages[0]), logical_page_to_block(pages[512]));

    for (i = 0; i < 513; ++i)
    {
        PagePoolFree(&ppool, pages[i]);
    }

    EXPECT_EQ(ppool.subPoolNum, 2);

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
    PagePoolGlobalUninit();
    kfree(pages);

    WearLevelerUninit(&wl);
    NVMUninit();
}