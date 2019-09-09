#ifndef IO_CONTROL_DEFINE_H
#define IO_CONTROL_DEFINE_H

#include<linux/ioctl.h>

#define UNIT_TEST_MAGIC 'u'

#define UNIT_TEST_GET_TESTS_COUNT _IO(UNIT_TEST_MAGIC, 1)
#define UNIT_TEST_GET_TEST_NAME _IO(UNIT_TEST_MAGIC, 2)
#define UNIT_TEST_RUN_TEST _IO(UNIT_TEST_MAGIC, 3)

#define MAX_NAME_LENGTH 64

struct UnitTestArgs
{
    char testCaseName[MAX_NAME_LENGTH];
    char testName[MAX_NAME_LENGTH];
    int index;
    int testCount;
};

#endif