#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kutest.h"

struct cmdPair
{
    const char * cmd;
    void(*func)(int, char *[]);
};

#define MAX_CMDS_NUM 4
struct cmds
{
    struct cmdPair * array;
};

void RunAllTests()
{
    int ret;
    int count;


    int success = 0;
    char testName[1024];
    char testCase[1024];

    count = KutestGetUnitTestCount();
    for(int i = 0; i < count; ++i)
    {
        KutestGetSpecifyUnitTestName(i, testCase, testName);
        printf("test case: %s, test name: %s,", testCase, testName);
        ret = KutestRunSpecifyUnitTest(i);
        if(ret == 0)
        {
            printf("passed\n");
            success++;
        }
        else
        {
            printf("failed\n");
        }
    }
    printf("total %d tests, passed %d tests\n", count, success);
}

void RunOneTest(int index)
{
    int ret;
    char testCase[1024];
    char testName[1024];
    
    KutestGetSpecifyUnitTestName(index, testCase, testName);
    printf("test case: %s, test name: %s,", testCase, testName);
    ret = KutestRunSpecifyUnitTest(index);
    if(ret == 0)
    {
        printf("passed\n");
    }
    else
    {
        printf("failed\n");
    }
}

void HandleRunCmd(int argc, char * argv[])
{
    char * index = argv[2];
    int i;

    if(strcmp(index,"all") == 0)
    {
        RunAllTests();
        return;
    }


    i = atoi(index);
    if(i == 0 && index[0] != '0')
    {
        printf("index %s is invalid\n", index);
        return;
    }
    if(i < 0)
    {
        printf("index %s is invalid\n", index);
        return;
    }
    RunOneTest(i);
}

void HandleListCmd(int argc, char * argv[])
{
    int testCount;

    testCount = KutestGetUnitTestCount();

    char testName[1024];
    char testCase[1024];

    printf("index\ttest case\t\ttest name\n");
    for(int i =0; i < testCount; ++i)
    {
        KutestGetSpecifyUnitTestName(i, testCase, testName);
        printf("%d\t%s\t%s\n", i, testCase, testName);
    }
}

void InitCmds(struct cmds * cmds)
{
    static struct cmdPair pairs[MAX_CMDS_NUM] = 
    {
        {"run", HandleRunCmd},
        {"r", HandleRunCmd},
        {"list", HandleListCmd},
        {"l", HandleListCmd}
    };
    cmds->array = pairs;
}

void usage()
{
    printf("usage : ./trigger option [index]\n");
    printf("usage : option=run : run specify index unit test(index=all: run all unit test)\n");
    printf("usage : option=list means list all tests\n");
}

int main(int argc, char * argv[])
{
    struct cmds cmds;
    int hit = 0;
    int i = 0;

    if(argc < 2)
    {
        usage();
        return 0;
    }

    InitCmds(&cmds);

    for(i = 0; i < MAX_CMDS_NUM; ++i)
    {
        if(strcmp(argv[1], cmds.array[i].cmd) == 0)
        {
            hit = 1;
            cmds.array[i].func(argc, argv);
            break;
        }
    }

    if(!hit)
    {
        usage();
    }

    return 0;
}