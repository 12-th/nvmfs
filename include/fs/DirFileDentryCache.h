#ifndef DIR_FILE_DENTRY_CACHE_H
#define DIR_FILE_DENTRY_CACHE_H

#include "Types.h"
#include <linux/hashtable.h>
#include <linux/rbtree.h>

struct CircularBuffer;

#define FILE_DENTRY_HASH_BIT 3

struct DentryNode
{
    nvmfs_ino_t ino;
    nvmfs_inode_type type;
    logic_addr_t nameAddr;
    UINT64 nameLen;
    struct hlist_node node;
};

struct DirFileDentryCache
{
    UINT64 totalDentryNameLen;
    DECLARE_HASHTABLE(hash, FILE_DENTRY_HASH_BIT);
};

struct DentryRecoveryNode
{
    nvmfs_ino_t ino;
    struct rb_node node;
};

struct DirFileDentryRecoveryCache
{
    struct rb_root root;
};

void DirFileDentryCacheInit(struct DirFileDentryCache * cache);
void DirFileDentryCacheUninit(struct DirFileDentryCache * cache);
struct DentryNode * DirFileDentryCacheLookup(struct DirFileDentryCache * cache, nvmfs_ino_t ino);
int DirFileDentryCacheAppendDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino, nvmfs_inode_type type,
                                   logic_addr_t nameAddr, UINT64 nameLen);
int DirFileDentryCacheRemoveDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino);
void DirFileDentryCacheDestroy(struct DirFileDentryCache * cache);

void DirFileDentryRecoveryCacheInit(struct DirFileDentryRecoveryCache * cr);
void DirFileDentryRecoveryCacheAdd(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino);
void DirFileDentryRecoveryCacheRemove(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino);
void DirFileDentryRecoveryCacheUninit(struct DirFileDentryRecoveryCache * cr, struct CircularBuffer * cb);

#endif