#include "CircularBuffer.h"
#include "DirFileDentryCache.h"
#include "Log.h"
#include "NVMAccesser.h"
#include <linux/slab.h>

void DirFileDentryCacheInit(struct DirFileDentryCache * cache)
{
    cache->totalDentryNameLen = 0;
    cache->dentryNum = 0;
    hash_init(cache->inohash);
    hash_init(cache->namehash);
}

void DirFileDentryCacheUninit(struct DirFileDentryCache * cache)
{
    int i;
    struct DentryNode * pos;
    struct hlist_node * next;
    hash_for_each_safe(cache->inohash, i, next, pos, inonode)
    {
        hlist_del(&pos->inonode);
        kfree(pos);
    }
}

struct DentryNode * DirFileDentryCacheLookupByIno(struct DirFileDentryCache * cache, nvmfs_ino_t ino)
{
    struct DentryNode * node;

    hash_for_each_possible(cache->inohash, node, inonode, ino)
    {
        if (node->ino == ino)
            return node;
    }

    return NULL;
}

static struct DentryNode * DirFileDentryCacheLookupByNameImpl(struct DirFileDentryCache * cache, char * name,
                                                              UINT32 nameHash, UINT64 nameLen, struct Log * log,
                                                              struct NVMAccesser * acc)
{
    struct DentryNode * node;
    char * buffer = NULL;

    hash_for_each_possible(cache->namehash, node, namenode, nameHash)
    {
        if (node->nameHash != nameHash)
            continue;
        if (node->nameLen != nameLen)
            continue;
        if (buffer == NULL)
            buffer = kmalloc(nameLen, GFP_KERNEL);
        LogRead(log, node->nameAddr, node->nameLen, buffer, acc);
        if (strcmp(buffer, name) == 0)
        {
            kfree(buffer);
            return node;
        }
    }
    if (buffer)
        kfree(buffer);

    return NULL;
}

struct DentryNode * DirFileDentryCacheLookupByName(struct DirFileDentryCache * cache, char * name, UINT32 nameHash,
                                                   UINT64 nameLen, struct Log * log, struct NVMAccesser * acc)
{
    return DirFileDentryCacheLookupByNameImpl(cache, name, nameHash, nameLen, log, acc);
}

int DirFileDentryCacheAppendDentryCheck(struct DirFileDentryCache * cache, nvmfs_ino_t ino, void * name,
                                        UINT32 nameHash, UINT32 nameLen, struct Log * log, struct NVMAccesser * acc)
{
    struct DentryNode * node;
    node = DirFileDentryCacheLookupByIno(cache, ino);
    if (node)
        return -EEXIST;
    node = DirFileDentryCacheLookupByNameImpl(cache, name, nameHash, nameLen, log, acc);
    if (node)
        return -EEXIST;
    return 0;
}

int DirFileDentryCacheJustAppendDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino, nvmfs_inode_type type,
                                       logic_addr_t nameAddr, UINT32 nameHash, UINT64 nameLen)
{
    struct DentryNode * node;

    node = kmalloc(sizeof(struct DentryNode), GFP_KERNEL);
    node->ino = ino;
    node->type = type;
    node->nameAddr = nameAddr;
    node->nameLen = nameLen;
    node->nameHash = nameHash;
    cache->totalDentryNameLen += nameLen;
    cache->dentryNum++;
    hash_add(cache->inohash, &node->inonode, ino);
    hash_add(cache->namehash, &node->namenode, nameHash);
    return 0;
}

int DirFileDentryCacheRemoveDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino)
{
    struct DentryNode * node;
    struct hlist_node * next;
    hash_for_each_possible_safe(cache->inohash, node, next, inonode, ino)
    {
        if (node->ino == ino)
        {
            cache->totalDentryNameLen -= node->nameLen;
            cache->dentryNum--;
            hash_del(&node->inonode);
            hash_del(&node->namenode);
            kfree(node);
            return 0;
        }
    }
    return -EINVAL;
}

void DirFileDentryCacheIterate(struct DirFileDentryCache * cache, UINT64 index,
                               int (*func)(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino),
                               void * data, struct Log * log, struct NVMAccesser * acc)
{
    int i;
    struct DentryNode * node;
    char * buffer = NULL;
    UINT32 bufferLen = 0;
    UINT64 num = 0;

    if (index >= cache->dentryNum)
        return;

    hash_for_each(cache->inohash, i, node, inonode)
    {
        int err;
        if (num < index)
        {
            num++;
            continue;
        }
        if (!buffer || bufferLen < node->nameLen)
        {
            kfree(buffer);
            buffer = kmalloc(node->nameLen, GFP_KERNEL);
            bufferLen = node->nameLen;
        }
        LogRead(log, node->nameAddr, node->nameLen, buffer, acc);
        err = func(data, node->type, buffer, node->nameLen, node->ino);
        if (err)
        {
            kfree(buffer);
            return;
        }
        num++;
    }
    kfree(buffer);
}

void DirFileDentryCacheDestroy(struct DirFileDentryCache * cache)
{
    int i;
    struct DentryNode * pos;
    struct hlist_node * next;
    hash_for_each_safe(cache->inohash, i, next, pos, inonode)
    {
        hash_del(&pos->inonode);
        kfree(pos);
    }
}

void DirFileDentryRecoveryCacheInit(struct DirFileDentryRecoveryCache * cr)
{
    cr->root = RB_ROOT;
}

void DirFileDentryRecoveryCacheAdd(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino)
{
    struct DentryRecoveryNode * node;
    struct rb_node ** new, *parent;

    parent = NULL;
    new = &(cr->root.rb_node);
    node = kmalloc(sizeof(struct DentryRecoveryNode), GFP_KERNEL);
    node->ino = ino;
    while (*new)
    {
        struct DentryRecoveryNode * this = container_of(*new, struct DentryRecoveryNode, node);
        parent = *new;
        if (node->ino < this->ino)
            new = &((*new)->rb_left);
        else
            new = &((*new)->rb_right);
    }
    rb_link_node(&node->node, parent, new);
    rb_insert_color(&node->node, &cr->root);
}

void DirFileDentryRecoveryCacheRemove(struct DirFileDentryRecoveryCache * cr, nvmfs_ino_t ino)
{
    struct rb_node * node = cr->root.rb_node;
    struct DentryRecoveryNode * cur = NULL;
    while (node)
    {
        cur = container_of(node, struct DentryRecoveryNode, node);
        if (ino == cur->ino)
            break;
        else if (ino < cur->ino)
            node = node->rb_left;
        else
            node = node->rb_right;
    }
    if (cur)
    {
        rb_erase(&cur->node, &cr->root);
        kfree(cur);
    }
}

void DirFileDentryRecoveryCacheUninit(struct DirFileDentryRecoveryCache * cr, struct CircularBuffer * cb)
{
    struct rb_node *cur, *next;
    struct DentryRecoveryNode * node;

    cur = rb_first(&cr->root);
    while (cur)
    {
        next = rb_next(cur);
        node = container_of(cur, struct DentryRecoveryNode, node);
        rb_erase(cur, &cr->root);
        CircularBufferAdd(cb, (void *)(node->ino));
        kfree(node);
        cur = next;
    }
}