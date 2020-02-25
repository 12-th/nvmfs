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

    if (argc != 2 && argc != 3)
    {
        printf("usage : cmd logicaddr path\n");
        printf("default path ~/tmp\n");
        return 0;
    }
    fd = open(argc != 3 ? "/home/mq/tmp" : argv[2], O_RDONLY);
    if (fd == -1)
        handle_error("open");

    unsigned long addr = strtoul(argv[1], NULL, 16);
    char * buf = malloc(1UL << 21);
    unsigned long ioctlarg[2];
    ioctlarg[0] = (unsigned long)addr;
    ioctlarg[1] = (unsigned long)buf;
    ioctl(fd, 0xf001, (unsigned long)&ioctlarg);
    printf("%s\n", buf);
    free(buf);

    exit(EXIT_SUCCESS);
    return 0;
}