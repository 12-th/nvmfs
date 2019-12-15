#ifndef NVMFS_INODE_TYPE_H
#define NVMFS_INODE_TYPE_H

#include <linux/fs.h>

#define S_IFMT (-1UL)
#define S_IFREG 1
#define S_IFDIR 2
#define S_IFLNK 3
#define DT_REG 1
#define DT_DIR 2
#define DT_LNK 3
#define DT_UNKNOWN -1

#define INODE_TYPE_REGULAR_FILE 1
#define INODE_TYPE_DIR_FILE 2
#define INODE_TYPE_LINK_FILE 3

static inline UINT8 ModeToType(umode_t mode)
{
    switch (mode & S_IFMT)
    {
    case S_IFREG:
        return INODE_TYPE_REGULAR_FILE;
        break;
    case S_IFDIR:
        return INODE_TYPE_DIR_FILE;
        break;
    case S_IFLNK:
        return INODE_TYPE_LINK_FILE;
        break;
    default:
        break;
    }
    return -1;
}

static inline UINT8 TypeToVfsDentryType(UINT8 type)
{
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        return DT_REG;
    case INODE_TYPE_DIR_FILE:
        return DT_DIR;
    case INODE_TYPE_LINK_FILE:
        return DT_LNK;
    }
    return DT_UNKNOWN;
}

#endif