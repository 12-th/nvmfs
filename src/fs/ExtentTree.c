#include "ExtentTree.h"
#include <linux/slab.h>

void ExtentTreeInit(struct ExtentTree * tree)
{
    tree->root = RB_ROOT;
}

void ExtentTreeUninit(struct ExtentTree * tree)
{
    struct rb_node * node = rb_first(&tree->root);
    while (node)
    {
        struct ExtentNode * extent;
        struct rb_node * cur = node;

        node = rb_next(cur);
        extent = rb_entry(cur, struct ExtentNode, node);
        rb_erase(&extent->node, &tree->root);
        kfree(extent);
    }
}

static inline int IsAddrInExtent(struct Extent * extent, UINT64 start)
{
    return start >= extent->start && start < extent->end;
}

static struct ExtentNode * ExtentTreeLowerBound(struct ExtentTree * tree, UINT64 addr)
{
    struct rb_node * node = tree->root.rb_node;
    struct rb_node * retNode = NULL;

    while (node)
    {
        struct ExtentNode * extent = container_of(node, struct ExtentNode, node);
        if (IsAddrInExtent(&extent->extent, addr))
        {
            return extent;
        }
        if (addr < extent->extent.start)
        {
            node = node->rb_left;
        }
        else
        {
            retNode = node;
            node = node->rb_right;
        }
    }
    if (retNode)
        return container_of(retNode, struct ExtentNode, node);
    return NULL;
}

static inline int CanMergeFront(struct ExtentNode * prev, UINT64 start)
{
    return prev && prev->extent.end == start;
}

static inline int CanMergeNext(struct ExtentNode * next, UINT64 end)
{
    return next && next->extent.start == end;
}

static void ExtentTreeJustInsert(struct ExtentTree * tree, struct ExtentNode * newExtent)
{
    struct rb_node ** new = &(tree->root.rb_node);
    struct rb_node * parent = NULL;

    while (*new)
    {
        struct ExtentNode * extent = container_of(*new, struct ExtentNode, node);
        parent = *new;
        if (newExtent->extent.start < extent->extent.start)
        {
            new = &((*new)->rb_left);
        }
        else if (newExtent->extent.start > extent->extent.start)
        {
            new = &((*new)->rb_right);
        }
        else
        {
            NOT_REACH_HERE();
        }
    }
    rb_link_node(&newExtent->node, parent, new);
    rb_insert_color(&newExtent->node, &tree->root);
}

void ExtentTreePut(struct ExtentTree * tree, UINT64 start, UINT64 end, gfp_t flags)
{
    struct rb_node * nextNode;
    struct ExtentNode *prev, *next, *newExtent;

    prev = ExtentTreeLowerBound(tree, start);
    if (prev)
        nextNode = rb_next(&prev->node);
    else
        nextNode = rb_first(&tree->root);

    if (nextNode)
        next = container_of(nextNode, struct ExtentNode, node);
    else
        next = NULL;
    if (CanMergeFront(prev, start) && CanMergeNext(next, end))
    {
        rb_erase(&next->node, &tree->root);
        kfree(next);
        prev->extent.end = next->extent.end;
        return;
    }
    if (CanMergeFront(prev, start))
    {
        prev->extent.end = end;
        return;
    }
    if (CanMergeNext(next, end))
    {
        rb_erase(&next->node, &tree->root);
        next->extent.start = start;
        ExtentTreeJustInsert(tree, next);
        return;
    }
    newExtent = kmalloc(sizeof(struct ExtentNode), flags);
    newExtent->extent.start = start;
    newExtent->extent.end = end;
    ExtentTreeJustInsert(tree, newExtent);
}

void ExtentContainerAppend(struct ExtentContainer * container, UINT64 start, UINT64 end, gfp_t flags)
{
    if (container->currentNum == container->extentNum)
    {
        struct Extent * newArray = kmalloc(sizeof(struct Extent) * container->extentNum * 2, flags);
        memcpy(newArray, container->array, sizeof(struct Extent) * container->extentNum);
        if (container->array != container->inlineArray)
        {
            kfree(container->array);
        }
        container->array = newArray;
        container->extentNum = container->extentNum * 2;
    }
    container->array[container->currentNum].start = start;
    container->array[container->currentNum].end = end;
    container->size += (end - start);
    container->currentNum++;
}

void ExtentTreeGet(struct ExtentTree * tree, struct ExtentContainer * container, UINT64 size, gfp_t flags)
{
    struct rb_node * node = rb_first(&tree->root);
    while (node && size)
    {
        struct ExtentNode * extent;
        struct rb_node * cur = node;

        node = rb_next(cur);
        extent = rb_entry(cur, struct ExtentNode, node);
        if (size >= (extent->extent.end - extent->extent.start))
        {
            ExtentContainerAppend(container, extent->extent.start, extent->extent.end, flags);
            rb_erase(&extent->node, &tree->root);
            size -= (extent->extent.end - extent->extent.start);
            kfree(extent);
        }
        else
        {
            ExtentContainerAppend(container, extent->extent.start, extent->extent.start + size, flags);
            rb_erase(&extent->node, &tree->root);
            extent->extent.start += size;
            ExtentTreeJustInsert(tree, extent);
            size = 0;
        }
    }
}

void ExtentTreeBatchPut(struct ExtentTree * tree, struct ExtentContainer * container, gfp_t flags)
{
    int i;
    for (i = 0; i < container->currentNum; ++i)
    {
        ExtentTreePut(tree, container->array[i].start, container->array[i].end, flags);
    }
}

void ExtentTreeReverseHolesAndExtents(struct ExtentTree * tree, UINT64 maxSize)
{
    struct rb_node *cur, *next;
    struct ExtentNode * extent;
    UINT64 lastHole = 0;
    struct ExtentTree newTree;

    ExtentTreeInit(&newTree);
    cur = rb_first(&tree->root);
    while (cur)
    {
        extent = container_of(cur, struct ExtentNode, node);
        if (lastHole < extent->extent.start)
        {
            ExtentTreePut(&newTree, lastHole, extent->extent.start, GFP_KERNEL);
        }
        lastHole = extent->extent.end;
        next = rb_next(cur);
        rb_erase(cur, &tree->root);
        kfree(extent);
        cur = next;
    }
    if (lastHole != maxSize)
        ExtentTreePut(&newTree, lastHole, maxSize, GFP_KERNEL);
    ExtentTreeUninit(tree);
    *tree = newTree;
}

void ExtentContainerInit(struct ExtentContainer * container, gfp_t flags)
{
    container->size = 0;
    container->extentNum = DEFAULT_EXTENT_CONTAINER_NUM;
    container->currentNum = 0;
    container->array = container->inlineArray;
}

void ExtentContainerUninit(struct ExtentContainer * container)
{
    container->size = 0;
    container->extentNum = DEFAULT_EXTENT_CONTAINER_NUM;
    container->currentNum = 0;
    if (container->array != container->inlineArray)
    {
        kfree(container->array);
        container->array = container->inlineArray;
    }
}

void ExtentContainerClear(struct ExtentContainer * container)
{
    container->currentNum = 0;
    container->size = 0;
}