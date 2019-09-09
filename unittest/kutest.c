#include "kutest.h"
#ifdef KUTEST_KERNEL_MODE
#include <linux/sort.h>
#include <linux/string.h>
#else
#include <stdlib.h>
#include <string.h>
#endif

extern struct kutest_info KUTEST_SECTION_START;
extern struct kutest_info KUTEST_SECTION_END;

struct kutest_result current_unit_test_result;
struct kutest_info * current_unit_test_info;

static int CompareKutestInfo(const void * a, const void * b)
{
    static int ret;

    struct kutest_info * ia = (struct kutest_info *)a;
    struct kutest_info * ib = (struct kutest_info *)b;
    ret = strcmp(ia->test_case, ib->test_case);
    if (ret)
        return ret;
    ret = strcmp(ia->test_name, ib->test_name);
    return ret;
}

static void SortUnitTests(struct kutest_info * start, struct kutest_info * end)
{
    static int is_sorted = 0;

    if (is_sorted)
        return;

#ifdef KUTEST_KERNEL_MODE
    sort(start, end - start, sizeof(struct kutest_info), CompareKutestInfo, NULL);
#else
    qsort(start, end - start, sizeof(struct kutest_info), CompareKutestInfo);
#endif

    is_sorted = 1;
}

int KutestGetSpecifyUnitTestName(int index, char * test_case, char * test_name)
{
    struct kutest_info * start = &KUTEST_SECTION_START;
    struct kutest_info * end = &KUTEST_SECTION_END;
    struct kutest_info * it;
    int unit_test_count = end - start;

    SortUnitTests(start, end);

    if (index >= unit_test_count)
    {
        kutest_print("kutest: index is out of range\n");
        return -1;
    }
    it = start + index;

    strcpy(test_case, it->test_case);
    strcpy(test_name, it->test_name);

    return 0;
}

int KutestGetUnitTestCount(void)
{
    return (&KUTEST_SECTION_END) - (&KUTEST_SECTION_START);
}

int KutestRunSpecifyUnitTest(int index)
{
    struct kutest_info * start = &KUTEST_SECTION_START;
    struct kutest_info * end = &KUTEST_SECTION_END;
    struct kutest_info * it;
    int unit_test_count = end - start;

    SortUnitTests(start, end);

    if (index >= unit_test_count)
    {
        kutest_print("kutest: index %d is out of range\n", index);
        return -1;
    }
    it = start + index;

    memset(&current_unit_test_result, 0, sizeof(current_unit_test_result));
    current_unit_test_info = it;

    kutest_print("kutest: run test, test case: %s, test name: %s\n", it->test_case, it->test_name);
    it->func();

    if (current_unit_test_result.normalFailed || current_unit_test_result.fatalFailed)
    {
        return -1;
    }

    return 0;
}