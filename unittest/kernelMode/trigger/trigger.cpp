#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include "../IOctlDefine.h"

using namespace std;

void RunAllTests(int fd)
{
    UnitTestArgs args;
    int ret;
    int count;

    ret = ioctl(fd, UNIT_TEST_GET_TESTS_COUNT, &args);
    if(ret < 0)
    {
        cerr << "something is error when call get_test_count" << endl;
        return;
    }

    count = args.testCount;
    int success = 0;
    for(int i = 0; i < count; ++i)
    {
        args.index = i;
        ret = ioctl(fd, UNIT_TEST_GET_TEST_NAME, &args);
        if(ret < 0)
        {
            cout << "something is error when call get_test_name" << endl;
            return;
        }
        cout << "test case: " << args.testCaseName << ", test name: " << args.testName << ", ";
        ret = ioctl(fd, UNIT_TEST_RUN_TEST, &args);
        if(ret == 0)
        {
            cout << "passed" << endl;
            success++;
        }
        else
        {
            cout << "failed" << endl;
        }
    }
    cout << "total " << count << " tests, passed " << success << " tests" << endl;
}

void RunOneTest(int fd, int index)
{
    UnitTestArgs args;
    int ret;

    args.index = index;
    ret = ioctl(fd, UNIT_TEST_GET_TEST_NAME, &args);
    if(ret < 0)
    {
        cout << "something is error when call get_test_name" << endl;
        return;
    }
    cout << "test case: " << args.testCaseName << ", test name: " << args.testName << ", ";
    ret = ioctl(fd, UNIT_TEST_RUN_TEST, &args);
    if(ret == 0)
    {
        cout << "passed" << endl;
    }
    else
    {
        cout << "failed" << endl;
    }
}

void HandleRunCmd(int fd, int argc, char * argv[])
{
    string index = argv[2];
    int i;

    if(index == "all")
    {
        RunAllTests(fd);
        return;
    }

    try
    {
        i = stoi(argv[2]);
    }
    catch(std::invalid_argument)
    {
        cerr << "index " << argv[2] << " is invalid" << endl;
        return;
    }
    if(i < 0)
    {
        cerr << "index " << i << " is invalid" << endl;
        return;
    }
    RunOneTest(fd, i);
}

void HandleListCmd(int fd, int argc, char * argv[])
{
    UnitTestArgs args;
    int ret;

    ret = ioctl(fd, UNIT_TEST_GET_TESTS_COUNT, &args);
    if(ret < 0)
    {
        cerr << "something is error when call get_test_count" << endl;
        return;
    }

    cout << "index\ttest case\t\ttest name" << endl;
    for(int i =0; i < args.testCount; ++i)
    {
        args.index = i;
        ret = ioctl(fd, UNIT_TEST_GET_TEST_NAME, &args);
        if(ret < 0)
        {
            cerr << "something is error when call get_test_name" << endl;
            return;
        }
        cout << i << "\t" << args.testCaseName << "\t" << args.testName << endl;
    }
}

void InitCmds(vector<pair<string, void(*)(int, int, char *[])>> & v)
{
    v.push_back(make_pair(string("run"), HandleRunCmd));
    v.push_back(make_pair(string("r"), HandleRunCmd));
    v.push_back(make_pair(string("list"), HandleListCmd));
    v.push_back(make_pair(string("l"), HandleListCmd));
}

void usage()
{
    cout << "usage : ./trigger option [index]"  << endl;
    cout << "usage : option=run : run specify index unit test(index=all: run all unit test)" << endl;
    cout << "usage : option=list means list all tests" << endl;
}

int main(int argc, char * argv[])
{
    int fd;

    if(argc < 2)
    {
        usage();
        return 0;
    }

    vector<pair<string, void(*)(int, int, char *[])>> cmds;
    InitCmds(cmds);

    fd = open("/dev/unit_tester", O_RDWR);
    if(fd < 0)
    {
        perror("open failed: ");
        return -1;
    }

    bool hit = false;

    for(auto it = cmds.begin(); it != cmds.end(); ++it)
    {
        if(it->first == argv[1])
        {
            hit = true;
            auto func = it->second;
            func(fd, argc, argv);
            break;
        }
    }

    if(!hit)
    {
        usage();
    }

    close(fd);
    return 0;
}