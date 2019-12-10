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

    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    page = logical_addr_to_page(PagePoolAlloc(&ppool));
    EXPECT_EQ(page, 0);
    PagePoolFree(&ppool, logical_addr_to_page(page));
    EXPECT_EQ(ppool.subPoolNum, 1);

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);

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
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    for (i = 0; i < 512; ++i)
    {
        pages[i] = logical_addr_to_page(PagePoolAlloc(&ppool));
    }

    for (i = 0; i < 511; i++)
    {
        EXPECT_TRUE(pages[i] + 1 == pages[i + 1]);
    }

    for (i = 0; i < 512; ++i)
    {
        PagePoolFree(&ppool, logical_addr_to_page(pages[i]));
    }

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
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
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    for (i = 0; i < 513; ++i)
    {
        pages[i] = logical_addr_to_page(PagePoolAlloc(&ppool));
    }

    for (i = 0; i < 511; i++)
    {
        EXPECT_EQ(logical_page_to_block(pages[i]), logical_page_to_block(pages[i + 1]));
    }

    EXPECT_NE(logical_page_to_block(pages[0]), logical_page_to_block(pages[512]));

    for (i = 0; i < 513; ++i)
    {
        PagePoolFree(&ppool, logical_addr_to_page(pages[i]));
    }

    EXPECT_EQ(ppool.subPoolNum, 2);

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
    kfree(pages);

    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(PagePoolTest, AllocWithHintTest)
{
    struct BlockPool bpool;
    struct PagePool ppool;
    logic_addr_t *pages1, *pages2;
    struct ExtentContainer container;
    UINT64 i;

    struct WearLeveler wl;
    struct NVMAccesser acc;

    NVMInit(1UL << 30);
    WearLevelerFormat(&wl, 30, 0);
    NVMAccesserInit(&acc, &wl);
    BlockPoolInit(&bpool);
    PagePoolInit(&ppool, &bpool, acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, 100, GFP_KERNEL);
    BlockPoolExtentPut(&bpool, &container);
    ExtentContainerUninit(&container);

    pages1 = kmalloc(sizeof(logical_page_t) * 512, GFP_KERNEL);
    pages2 = kmalloc(sizeof(logical_page_t) * 512, GFP_KERNEL);

    for (i = 0; i < 512; ++i)
    {
        pages1[i] = PagePoolAlloc(&ppool);
    }
    pages2[0] = PagePoolAlloc(&ppool);

    for (i = 0; i < 512; ++i)
    {
        PagePoolFree(&ppool, pages1[i]);
    }

    for (i = 1; i < 512; ++i)
    {
        pages2[i] = PagePoolAllocWithHint(&ppool, pages2[i - 1]);
    }

    for (i = 0; i < 511; ++i)
    {
        EXPECT_EQ(logical_addr_to_block(pages2[i]), logical_addr_to_block(pages2[i + 1]));
    }

    PagePoolUninit(&ppool);
    BlockPoolUninit(&bpool);
    kfree(pages1);
    kfree(pages2);
    WearLevelerUninit(&wl);
    NVMUninit();
}