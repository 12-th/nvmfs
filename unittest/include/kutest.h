#ifndef KERNEL_UNIT_TEST_H
#define KERNEL_UNIT_TEST_H

#define KUTEST_SECTION_NAME kutest
#define KUTEST_SECTION_NAME_STR "kutest"
#define KUTEST_SECTION_START __start_kutest
#define KUTEST_SECTION_END __stop_kutest

#ifdef KUTEST_KERNEL_MODE
#include <linux/printk.h>
#else
#include <stdio.h>
#endif

struct kutest_result
{
    int fatalFailed;
    int normalFailed;
};

struct kutest_info;

typedef void (*KutestFunc)(void);

struct kutest_info
{
    KutestFunc func;
    const char * test_case;
    const char * test_name;
};

#define TEST(TestCaseName, TestName)                                                                                   \
    static void Kutest_##TestCaseName##_##TestName(void);                                                              \
    struct kutest_info Kutest_info_##TestCaseName##_##TestName                                                         \
        __attribute__((section(KUTEST_SECTION_NAME_STR), aligned(8), unused)) = {                                      \
            .func = Kutest_##TestCaseName##_##TestName, .test_case = #TestCaseName, .test_name = #TestName};           \
    static void Kutest_##TestCaseName##_##TestName(void)

#ifdef KUTEST_KERNEL_MODE
#define kutest_fail_print(failed_type, fmt, ...)                                                                       \
    printk(KERN_ERR #failed_type " failed\t%s::%s\t %s:%d\t\t" fmt "\n", current_unit_test_info->test_case,            \
           current_unit_test_info->test_name, __FILE__, __LINE__, ##__VA_ARGS__)

#define kutest_print(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define kutest_fail_print(failed_type, fmt, ...)                                                                       \
    fprintf(stderr, "\033[40;31m" #failed_type " failed\t%s::%s\t %s:%d\t\t" fmt "\033[0m\n",                          \
            current_unit_test_info->test_case, current_unit_test_info->test_name, __FILE__, __LINE__, ##__VA_ARGS__)

#define kutest_print(fmt, ...) // printf(fmt, ##__VA_ARGS__)

#endif

extern struct kutest_result current_unit_test_result;
extern struct kutest_info * current_unit_test_info;

// void RunAllUnitTests(void);
// void RunSpecifiedTests(int startIndex, int num);
// void ListAllTests(void);

int KutestGetSpecifyUnitTestName(int index, char * test_case, char * test_name);
int KutestGetUnitTestCount(void);
int KutestRunSpecifyUnitTest(int index);

#define EXPECT_EQ(a, b)                                                                                                \
    {                                                                                                                  \
        typeof(a) _a = (a);                                                                                            \
        typeof(b) _b = (b);                                                                                            \
        if (_a != _b)                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(EXPECT_EQ, "%s != %s", #a, #b);                                                          \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define EXPECT_NE(a, b)                                                                                                \
    {                                                                                                                  \
        typeof(a) _a = (a);                                                                                            \
        typeof(b) _b = (b);                                                                                            \
        if (_a == _b)                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(EXPECT_NE, "%s != %s", #a, #b);                                                          \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define ASSERT_EQ(a, b)                                                                                                \
    {                                                                                                                  \
        typeof(a) _a = (a);                                                                                            \
        typeof(b) _b = (b);                                                                                            \
        if (_a != _b)                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(ASSERT_EQ, "%s != %s", #a, #b);                                                          \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define ASSERT_NE(a, b)                                                                                                \
    {                                                                                                                  \
        typeof(a) _a = (a);                                                                                            \
        typeof(b) _b = (b);                                                                                            \
        if (_a == _b)                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(ASSERT_NE, "%s != %s", #a, #b);                                                          \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define EXPECT_TRUE(expr)                                                                                              \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            kutest_fail_print(EXPECT_TRUE, "%s is false", #expr);                                                      \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define EXPECT_FALSE(expr)                                                                                             \
    {                                                                                                                  \
        if ((expr))                                                                                                    \
        {                                                                                                              \
            kutest_fail_print(EXPECT_FALSE, "%s is true", #expr);                                                      \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define ASSERT_TRUE(expr)                                                                                              \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            kutest_fail_print(ASSERT_TRUE, "%s is false", #expr);                                                      \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define ASSERT_FALSE(expr)                                                                                             \
    {                                                                                                                  \
        if ((expr))                                                                                                    \
        {                                                                                                              \
            kutest_fail_print(ASSERT_FALSE, "%s is true", #expr);                                                      \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define EXPECT_MEMEQ(a, b, size)                                                                                       \
    {                                                                                                                  \
        int index = 0;                                                                                                 \
        char * _a = (char *)(a);                                                                                       \
        char * _b = (char *)(b);                                                                                       \
        for (index = 0; index < size; ++index)                                                                         \
        {                                                                                                              \
            if (_a[index] != _b[index])                                                                                \
            {                                                                                                          \
                kutest_fail_print(EXPECT_MEM_EQ, "%s != %s, at index %d", #a, #b, index);                              \
                current_unit_test_result.normalFailed = 1;                                                             \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }

#define EXPECT_NULL(expr)                                                                                              \
    {                                                                                                                  \
        if (!!(expr))                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(ASSERT_NULL, "%s is not null", #expr);                                                   \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define ASSERT_NULL(expr)                                                                                              \
    {                                                                                                                  \
        if (!!(expr))                                                                                                  \
        {                                                                                                              \
            kutest_fail_print(ASSERT_NULL, "%s is not null", #expr);                                                   \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define EXPECT_NOT_NULL(expr)                                                                                          \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            kutest_fail_print(ASSERT_NULL, "%s is not null", #expr);                                                   \
            current_unit_test_result.normalFailed = 1;                                                                 \
        }                                                                                                              \
    }

#define ASSERT_NOT_NULL(expr)                                                                                          \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            kutest_fail_print(ASSERT_NULL, "%s is not null", #expr);                                                   \
            current_unit_test_result.fatalFailed = 1;                                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    }

#endif
