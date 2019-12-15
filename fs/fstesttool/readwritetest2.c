#include <dirent.h> /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define handle_error(msg)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define BUF_SIZE 4096

int main(int argc, char * argv[])
{
    int fd1, fd2, nread1, nread2;
    int offset = 0;
    char buf1[BUF_SIZE], buf2[BUF_SIZE];

    if (argc != 3)
    {
        printf("usage : ./a.out path1 path2");
        return 0;
    }

    fd1 = open(argv[1], O_RDWR);
    if (fd1 < 0)
        handle_error("open path1:");
    fd2 = open(argv[2], O_RDWR);
    if (fd2 < 0)
        handle_error("open path2:");

    do
    {
        int minSize = nread1 < nread2 ? nread1 : nread2;
        nread1 = read(fd1, buf1, BUF_SIZE);
        nread2 = read(fd2, buf2, BUF_SIZE);
        for (int i = 0; i < minSize; ++i)
        {
            if (buf1[i] != buf2[i])
            {
                printf("not same, start is %ld\n", (unsigned long)offset);
            }
            offset++;
        }
    } while (nread1 && nread2);

    return 0;
}