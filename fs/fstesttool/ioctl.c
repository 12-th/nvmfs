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

    if (argc < 3)
    {
        printf("usage : cmd path cmdno\n");
        return 0;
    }
    fd = open(argc > 1 ? argv[1] : "~/tmp", O_RDONLY);
    if (fd == -1)
        handle_error("open");

    if (argv[2][0] == '0')
        ioctl(fd, 0xf001, 0);
    else
        ioctl(fd, 0xf002, 0);

    exit(EXIT_SUCCESS);
    return 0;
}