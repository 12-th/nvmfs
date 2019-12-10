#include "CircularBuffer.h"
#include "DirFileDentryCache.h"
#include "Log.h"
#include <linux/slab.h>

void DirFileDentryCacheInit(struct DirFileDentryCache * cache)
{
    cache->totalDentryNameLen = 0;
    hash_init(cache->hash);
}

void DirFileDentryCacheUninit(struct DirFileDentryCache * cache)
{
    int i;
    struct DentryNode * pos;
    struct hlist_node * next;
    hash_for_each_safe(cache->hash, i, next, pos, node)
    {
        hlist_del(&pos->node);
        kfree(pos);
    }
}

struct DentryNode * DirFileDentryCacheLookup(struct DirFileDentryCache * cache, nvmfs_ino_t ino)
{
    struct DentryNode * node;

    hash_for_each_possible(cache->hash, node, node, ino)
    {
        if (node->ino == ino)
            return node;
    }

    return NULL;
}

int DirFileDentryCacheAppendDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino, nvmfs_inode_type type,
                                   logic_addr_t nameAddr, UINT64 nameLen)
{
    struct DentryNode * node;

    node = DirFileDentryCacheLookup(cache, ino);
    if (node)
        return -EEXIST;
    node = kmalloc(sizeof(struct DentryNode), GFP_KERNEL);
    node->ino = ino;
    node->type = type;
    node->nameAddr = nameAddr;
    node->nameLen = nameLen;
    cache->totalDentryNameLen += nameLen;
    hash_add(cache->hash, &node->node, ino);
    return 0;
}

int DirFileDentryCacheRemoveDentry(struct DirFileDentryCache * cache, nvmfs_ino_t ino)
{
    struct DentryNode * node;
    struct hlist_node * next;
    hash_for_each_possible_safe(cache->hash, node, next, node, ino)
    {
        if (node->ino == ino)
        {
            cache->totalDentryNameLen -= node->nameLen;
            hash_del(&node->node);
            kfree(node);
            return 0;
        }
    }
    return -EINVAL;
}

void DirFileDentryCacheDestroy(struct DirFileDentryCache * cache)
{
    int i;
    struct DentryNode * pos;
    struct hlist_node * next;
    hash_for_each_safe(cache->hash, i, next, pos, node)
    {
        hash_del(&pos->node);
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
    struct DentryRecoveryNode * cur;
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