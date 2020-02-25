#include "NVMAccesser.h"
#include "NVMOperations.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>

#define INIT()                                                                                                         \
    struct WearLeveler * wl;                                                                                           \
    struct NVMAccesser acc;                                                                                            \
    NVMInit(1UL << 30);                                                                                                \
    wl = kmalloc(sizeof(struct WearLeveler), GFP_KERNEL);                                                              \
    WearLevelerFormat(wl, 30, 0);                                                                                      \
    NVMAccesserInit(&acc, wl);

#define UNINIT()                                                                                                       \
    NVMAccesserUninit(&acc);                                                                                           \
    WearLevelerUninit(wl);                                                                                             \
    kfree(wl);                                                                                                         \
    NVMUninit();

TEST(WearLevelerTest, test1)
{
    INIT();

    UNINIT();
}

#define WRITE_DATA(magic, repeatTimes, addr, increaseWearCount)                                                        \
    for (i = 0; i < BUF_SIZE; ++i)                                                                                     \
    {                                                                                                                  \
        buf[i] = magic;                                                                                                \
    }                                                                                                                  \
    for (i = 0; i < repeatTimes; ++i)                                                                                  \
    {                                                                                                                  \
        NVMAccesserWrite(&acc, addr, sizeof(buf), buf, increaseWearCount);                                             \
    }

#define READ_AND_EXPECT(magic, addr)                                                                                   \
    NVMAccesserRead(&acc, addr, sizeof(buf), buf);                                                                     \
    for (i = 0; i < BUF_SIZE; ++i)                                                                                     \
    {                                                                                                                  \
        EXPECT_EQ(buf[i], magic);                                                                                      \
    }

TEST(WearLevelerTest, test2)
{
    const int BUF_SIZE = 32;
    int buf[BUF_SIZE];
    int i;
    INIT();

    WRITE_DATA(0xabcdef00, 102400, 0, 100);

    READ_AND_EXPECT(0xabcdef00, 0);

    UNINIT();
}

TEST(WearLevelerTest, test3)
{
    const int BUF_SIZE = 32;
    int buf[BUF_SIZE];
    int i;
    INIT();

    WRITE_DATA(0xabcdef00, 102400, 0, 100);
    WRITE_DATA(0x00fedcba, 102400, SIZE_2M, 100);

    READ_AND_EXPECT(0xabcdef00, 0);
    READ_AND_EXPECT(0x00fedcba, SIZE_2M);

    UNINIT();
}

TEST(WearLevelerTest, test4)
{
    const int BUF_SIZE = 32;
    int buf[BUF_SIZE];
    int i;
    INIT();

    NVMAccesserSplit(&acc, 0);
    WRITE_DATA(0xabcdef00, 102400, 0, 100);
    WRITE_DATA(0x00fedcba, 102400, SIZE_4K, 100);

    READ_AND_EXPECT(0xabcdef00, 0);
    READ_AND_EXPECT(0x00fedcba, SIZE_4K);

    UNINIT();
}

TEST(WearLevelerTest, test5)
{
    const int BUF_SIZE = 32;
    int buf[BUF_SIZE];
    int i;
    INIT();

    NVMAccesserSplit(&acc, 0);
    NVMAccesserSplit(&acc, SIZE_2M);

    WRITE_DATA(0xabcdef00, 102400, 0, 100);
    WRITE_DATA(0x00fedcba, 102400, SIZE_4K, 100);

    READ_AND_EXPECT(0xabcdef00, 0);
    READ_AND_EXPECT(0x00fedcba, SIZE_4K);

    UNINIT();
}
TEST(WearLevelerTest, test6)
{
    const int BUF_SIZE = 32;
    int buf[BUF_SIZE];
    int i;
    INIT();

    NVMAccesserSplit(&acc, 0);
    NVMAccesserSplit(&acc, SIZE_2M);

    WRITE_DATA(0xabcdef00, 102400, 0, 100);
    WRITE_DATA(0x00fedcba, 102400, SIZE_4K, 100);
    WRITE_DATA(0x12345678, 102400, SIZE_2M, 100);

    READ_AND_EXPECT(0xabcdef00, 0);
    READ_AND_EXPECT(0x00fedcba, SIZE_4K);
    READ_AND_EXPECT(0x12345678, SIZE_2M);

    UNINIT();
}