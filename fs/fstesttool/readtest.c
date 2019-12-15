#include <dirent.h> /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define handle_error(msg)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define BUF_SIZE 1024

int main(int argc, char * argv[])
{
    int fd, nread;
    char buf[BUF_SIZE];

    fd = open(argc > 1 ? argv[1] : ".", O_RDONLY);
    if (fd == -1)
        handle_error("open");

    while ((nread = read(fd, buf, BUF_SIZE)) >= 0)
    {
        printf("%s", buf);
    }

    exit(EXIT_SUCCESS);
    return 0;
}