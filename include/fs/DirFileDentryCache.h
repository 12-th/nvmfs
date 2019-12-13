#ifndef DIR_FILE_DENTRY_CACHE_H
#define DIR_FILE_DENTRY_CACHE_H

#include "Types.h"
#include <linux/hashtable.h>
#include <linux/rbtree.h>

struct CircularBuffer;
struct Log;
struct NVMAccesser;

#define FILE_DENTRY_HASH_BIT 3

struct DentryNode
{
    nvmfs_ino_t ino;
    nvmfs_inode_type type;
    logic_addr_t nameAddr;
    UINT64 nameLen;
    UINT32 nameHash;
    struct hlist_node inonode;
    struct hlist_node namenode;
};

struct DirFileDentryCache
{
    UINT64 totalDentryNameLen;
    UINT64 dentryNum;
    DECLARE_HASHTABLE(inohash, FILE_DENTRY_HASH_BIT);
    DECLARE_HASHTABLE(namehash, FILE_DENTRY_HASH_BIT);
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
struct DentryNode * DirFileDentryCacheLookupByIno(struct DirFileDentryCache * cache, nvmfs_ino_t ino);
struct DentryNode * DirFileDentryCacheLookupByName(struct DirFileDentryCache * cache, char * name, UINT32 nameHash,
                                                   UINT64 nameLen, struct Log * log, struct NVMAccesser * acc);
int DirFileDentryCacheAppendDentryCheck(struct DirFileDentryCache * cache, nvmfs_ino_t ino, void * name,
                                        UINT32 nameHash, UINT32 nameLen, struct Log * log, struct NVMAccesser * acc);
int DirFileDentryCacheJustAppendDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino, nvmfs_inode_type type,
                                       logic_addr_t nameAddr, UINT32 nameHash, UINT64 nameLen);
int DirFileDentryCacheRemoveDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino);
void DirFileDentryCacheIterate(struct DirFileDentryCache * cache, UINT64 index,
                               int (*func)(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino),
                               void * data, struct Log * log, struct NVMAccesser * acc);
void DirFileDentryCacheDestroy(struct DirFileDentryCache * cache);

void DirFileDentryRecoveryCacheInit(struct DirFileDentryRecoveryCache * cr);
void DirFileDentryRecoveryCacheAdd(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino);
void DirFileDentryRecoveryCacheRemove(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino);
void DirFileDentryRecoveryCacheUninit(struct DirFileDentryRecoveryCache * cr, struct CircularBuffer * cb);

#endif