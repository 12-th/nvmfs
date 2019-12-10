#include "ExtentTree.h"
#include "UnitTestHelp.h"
#include "kutest.h"

TEST(ExtentTreeTest, GetPutTest1)
{
    struct ExtentTree tree;
    struct ExtentContainer container;
    struct Extent * extent;

    ExtentTreeInit(&tree);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentTreePut(&tree, 0, 10, GFP_KERNEL);
    ExtentTreeGet(&tree, &container, 5, GFP_KERNEL);
    for_each_extent_in_container(extent, &container)
    {
        EXPECT_EQ(extent->start, 0);
        EXPECT_EQ(extent->end, 5);
    }
    ExtentContainerUninit(&container);
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, GetPutTest2)
{
    struct ExtentTree tree;
    struct ExtentContainer container;
    struct Extent * extent;

    ExtentTreeInit(&tree);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentTreePut(&tree, 0, 10, GFP_KERNEL);
    ExtentTreeGet(&tree, &container, 11, GFP_KERNEL);
    for_each_extent_in_container(extent, &container)
    {
        EXPECT_EQ(extent->start, 0);
        EXPECT_EQ(extent->end, 10);
    }
    EXPECT_EQ(container.size, 10);
    ExtentContainerUninit(&container);
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, GetPutTest3)
{
    struct ExtentTree tree;
    struct ExtentContainer container;
    struct Extent * extent;
    int i = 0;

    ExtentTreeInit(&tree);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentTreePut(&tree, 0, 3, GFP_KERNEL);
    ExtentTreePut(&tree, 5, 10, GFP_KERNEL);
    ExtentTreeGet(&tree, &container, 10, GFP_KERNEL);
    for_each_extent_in_container(extent, &container)
    {
        if (!i)
        {
            EXPECT_EQ(extent->start, 0);
            EXPECT_EQ(extent->end, 3);
        }
        else
        {
            EXPECT_EQ(extent->start, 5);
            EXPECT_EQ(extent->end, 10);
        }
        ++i;
    }
    EXPECT_EQ(container.size, 8);
    ExtentContainerUninit(&container);
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, ExtetnContainerTest)
{
    UINT64 arrayNum = 100;
    struct Extent * array;
    int i;
    struct ExtentContainer container;

    array = kmalloc(sizeof(struct Extent) * arrayNum, GFP_KERNEL);
    for (i = 0; i < arrayNum; ++i)
    {
        array[i].start = GetRandNumber();
        array[i].end = GetRandNumber();
    }

    ExtentContainerInit(&container, GFP_KERNEL);
    for (i = 0; i < arrayNum; ++i)
    {
        ExtentContainerAppend(&container, array[i].start, array[i].end, GFP_KERNEL);
    }
    ExtentContainerUninit(&container);
    kfree(array);
}

TEST(ExtentTreeTest, reverseTest1)
{
    struct ExtentTree tree;
    struct ExtentNode * extent;
    struct rb_node * rbnode;

    ExtentTreeInit(&tree);
    ExtentTreeReverseHolesAndExtents(&tree, 100);
    for_each_extent_in_extent_tree(extent, rbnode, &tree)
    {
        EXPECT_EQ(extent->extent.start, 0);
        EXPECT_EQ(extent->extent.end, 100);
    }
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, reverseTest2)
{
    struct ExtentTree tree;
    struct ExtentNode * extent;
    struct rb_node * rbnode;

    ExtentTreeInit(&tree);
    ExtentTreePut(&tree, 0, 100, GFP_KERNEL);
    ExtentTreeReverseHolesAndExtents(&tree, 100);
    for_each_extent_in_extent_tree(extent, rbnode, &tree)
    {
        EXPECT_EQ(extent, NULL);
        EXPECT_TRUE(0);
    }
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, reverseTest3)
{
    struct ExtentTree tree;
    struct ExtentNode * extent;
    struct rb_node * rbnode;
    int i = 0;

    ExtentTreeInit(&tree);
    ExtentTreePut(&tree, 0, 10, GFP_KERNEL);
    ExtentTreePut(&tree, 20, 50, GFP_KERNEL);
    ExtentTreePut(&tree, 70, 90, GFP_KERNEL);
    ExtentTreeReverseHolesAndExtents(&tree, 100);
    for_each_extent_in_extent_tree(extent, rbnode, &tree)
    {
        if (i == 0)
        {
            EXPECT_EQ(extent->extent.start, 10);
            EXPECT_EQ(extent->extent.end, 20);
        }
        else if (i == 1)
        {
            EXPECT_EQ(extent->extent.start, 50);
            EXPECT_EQ(extent->extent.end, 70);
        }
        else if (i == 2)
        {
            EXPECT_EQ(extent->extent.start, 90);
            EXPECT_EQ(extent->extent.end, 100);
        }
        else
        {
            EXPECT_TRUE(0);
        }
        ++i;
    }
    // expect 10-20,50-70,90-100
    ExtentTreeUninit(&tree);
}

TEST(ExtentTreeTest, reverseTest4)
{
    struct ExtentTree tree;
    struct ExtentNode * extent;
    struct rb_node * rbnode;
    int i = 0;

    ExtentTreeInit(&tree);
    ExtentTreePut(&tree, 20, 50, GFP_KERNEL);
    ExtentTreePut(&tree, 70, 100, GFP_KERNEL);
    ExtentTreeReverseHolesAndExtents(&tree, 100);
    for_each_extent_in_extent_tree(extent, rbnode, &tree)
    {
        if (i == 0)
        {
            EXPECT_EQ(extent->extent.start, 0);
            EXPECT_EQ(extent->extent.end, 20);
        }
        else if (i == 1)
        {
            EXPECT_EQ(extent->extent.start, 50);
            EXPECT_EQ(extent->extent.end, 70);
        }
        else
        {
            EXPECT_TRUE(0);
        }
        ++i;
    }
    // expect 0-20,50-70
    ExtentTreeUninit(&tree);
}