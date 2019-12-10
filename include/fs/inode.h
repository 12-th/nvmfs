#ifndef NVMFS_INODE_H
#define NVMFS_INODE_H

#include "Types.h"

struct FsConstructor;
struct CircularBuffer;

#define INODE_TYPE_REGULAR_FILE 1
#define INODE_TYPE_DIR_FILE 2
#define INODE_TYPE_LINK_FILE 3

struct NvmInode
{
    struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;
    UINT32 linkCount;
    UINT8 type;
    nvmfs_ino_t ino;
};

void InodeRecovery(logic_addr_t inode, struct FsConstructor * ctor, struct CircularBuffer * cb,
                   struct NVMAccesser * acc);

#endif