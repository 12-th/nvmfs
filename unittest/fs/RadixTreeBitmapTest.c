#include "kutest.h"
#include "RadixTreeBitmap.h"

TEST(RadixTreeBitmapTest, SetTest)
{
    struct RadixTreeBitmap map;
    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 0);
    
    EXPECT_TRUE(RadixTreeBitmapQuery(&map, 0));

    UninitRadixTreeBitmap(&map);
}

TEST(RadixTreeBitmapTest, ClearTest)
{
    struct RadixTreeBitmap map;
    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 42);
    RadixTreeBitmapClear(&map, 42);
    
    EXPECT_FALSE(RadixTreeBitmapQuery(&map, 42));
    EXPECT_TRUE(IsRadixTreeBitmapEmpty(&map));

    UninitRadixTreeBitmap(&map);
}

TEST(RadixTreeBitmapTest, findFirstSetBitTest1)
{
    struct RadixTreeBitmap map;
    unsigned int index;

    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 41);
    RadixTreeBitmapSet(&map, 43);
    index = RadixTreeBitmapFindFirstSetBitIndex(&map, 42);

    EXPECT_EQ(index, 43);

    UninitRadixTreeBitmap(&map);
}

TEST(RadixTreeBitmapTest, findFirstSetBitTest2)
{
    struct RadixTreeBitmap map;
    unsigned int index;

    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 41);
    RadixTreeBitmapSet(&map, 43);
    index = RadixTreeBitmapFindFirstSetBitIndex(&map, 44);

    EXPECT_EQ(index, RADIX_TREE_BITMAP_INVALID_INDEX);

    UninitRadixTreeBitmap(&map);
}

TEST(RadixTreeBitmapTest, findLastSetBitTest1)
{
    struct RadixTreeBitmap map;
    unsigned int index;

    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 41);
    RadixTreeBitmapSet(&map, 43);
    index = RadixTreeBitmapFindLastSetBitIndex(&map, 42);

    EXPECT_EQ(index, 41);

    UninitRadixTreeBitmap(&map);
}

TEST(RadixTreeBitmapTest, findLastSetBitTest2)
{
    struct RadixTreeBitmap map;
    unsigned int index;

    InitRadixTreeBitmap(&map);

    RadixTreeBitmapSet(&map, 41);
    RadixTreeBitmapSet(&map, 43);
    index = RadixTreeBitmapFindLastSetBitIndex(&map, 40);

    EXPECT_EQ(index, RADIX_TREE_BITMAP_INVALID_INDEX);

    UninitRadixTreeBitmap(&map);
}