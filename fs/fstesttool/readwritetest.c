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

#define BUF_SIZE 8192

int main(int argc, char * argv[])
{
    int fd, nread;
    char * buf;
    int offset = 0;
    char magic = 0xab;

    if (argc != 2)
    {
        printf("usage : ./a.out path");
        return 0;
    }
    buf = (int *)malloc(BUF_SIZE);
    buf[0] = buf[4097] = 0xff;
    memset(buf, magic, BUF_SIZE);
    fd = open(argv[1], O_RDWR);
    if (fd == -1)
        handle_error("open");

    write(fd, buf, BUF_SIZE);
    while ((nread = read(fd, buf, 4096)) > 0)
    {
        if (buf[0] != 0xff)
        {
            printf("incorrect read result,start byte is %d", offset);
            return 0;
        }
        for (int i = 1; i < 4096 / sizeof(char); ++i)
        {
            if (buf[i] != magic)
            {
                printf("incorrect read result,start byte is %d", offset);
                return 0;
            }
            offset += sizeof(char);
        }
    }

    exit(EXIT_SUCCESS);
    return 0;
}