#include <dirent.h> /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define handle_error(msg)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

int main(int argc, char * argv[])
{
    int fd;

    if (argc != 2 && argc != 1)
    {
        printf("usage : cmd path\n");
        printf("or\n");
        printf("usage : cmd (default path is ~/tmp)\n");
        return 0;
    }
    fd = open(argc == 1 ? "/home/mq/tmp" : argv[1], O_RDONLY);
    if (fd == -1)
        handle_error("open");

    char * buf = malloc(4096);
    ioctl(fd, 0xf003, (unsigned long)buf);
    printf("%s\n", buf);
    free(buf);

    exit(EXIT_SUCCESS);
    return 0;
}