#include "CircularBuffer.h"
#include "UnitTestHelp.h"
#include "kutest.h"
#include <linux/slab.h>

TEST(CircularBufferTest, wholeTest1)
{
    void ** data;
    void ** data1;
    struct CircularBuffer cb;
    UINT64 i;

    data = (void **)GenRandomArray64(512);
    data1 = kmalloc(sizeof(void *) * 512, GFP_KERNEL);
    CircularBufferInit(&cb);
    for (i = 0; i < 512; ++i)
    {
        CircularBufferAdd(&cb, data[i]);
    }

    for (i = 0; i < 512; ++i)
    {
        data1[i] = CircularBufferDelete(&cb);
    }

    for (i = 0; i < 512; ++i)
    {
        EXPECT_EQ(data[i], data1[i]);
    }
    EXPECT_TRUE(CircularBufferIsEmpty(&cb));

    CircularBufferUninit(&cb);
    kfree(data);
    kfree(data1);
}