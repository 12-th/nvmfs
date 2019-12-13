#include "FileExtentTree.h"
#include "Types.h"
#include <linux/slab.h>

void FileExtentTreeInit(struct FileExtentTree * tree)
{
    tree->fileExtentRoot = RB_ROOT;
    tree->spaceRoot = RB_ROOT;
    tree->effectiveSize = 0;
    tree->spaceSize = 0;
}

int FileExtentTreeAddSpace(struct FileExtentTree * tree, logic_addr_t addr, UINT64 size, gfp_t flags)
{
    struct rb_node ** new = &tree->spaceRoot.rb_node;
    struct rb_node * parent = NULL;
    struct FileSpaceNode * newNode;

    newNode = kmalloc(sizeof(struct FileSpaceNode), flags);
    if (!newNode)
        return -ENOMEM;
    newNode->start = addr;
    newNode->size = size;
    newNode->unmapRoot = RB_ROOT;

    while (*new)
    {
        struct FileSpaceNode * cur = container_of(*new, struct FileSpaceNode, node);
        parent = *new;
        if (newNode->start < cur->start)
            new = &((*new)->rb_left);
        else
            new = &((*new)->rb_right);
    }
    rb_link_node(&newNode->node, parent, new);
    rb_insert_color(&newNode->node, &tree->fileExtentRoot);

    tree->spaceSize += size;
    return 0;
}

static struct FileSpaceNode * LookupFileSpaceNode(struct FileExtentTree * tree, logic_addr_t addr)
{
    struct rb_node * node = tree->spaceRoot.rb_node;

    while (node)
    {
        struct FileSpaceNode * space = container_of(node, struct FileSpaceNode, node);
        if (addr >= space->start && addr < (space->start + space->size))
            return space;
        if (addr < space->start)
        {
            node = node->rb_left;
        }
        else
        {
            node = node->rb_right;
        }
    }
    return NULL;
}

static struct FileExtentNode * ExtentTreeLowerBound(struct FileExtentTree * tree, UINT64 start)
{
    struct rb_node * node = tree->fileExtentRoot.rb_node;
    struct rb_node * retNode = NULL;

    while (node)
    {
        struct FileExtentNode * extent = container_of(node, struct FileExtentNode, extentNode);
        if (start == extent->fileStart)
            return extent;
        if (start < extent->fileStart)
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
        return container_of(retNode, struct FileExtentNode, extentNode);
    return NULL;
}

static void DeleteExtentNode(struct FileExtentNode * node, struct FileExtentTree * tree)
{
    struct FileSpaceNode * spaceNode;

    spaceNode = LookupFileSpaceNode(tree, node->start);
    rb_erase(&node->extentNode, &tree->fileExtentRoot);
    rb_erase(&node->spaceNode, &spaceNode->unmapRoot);
}

static void DeleteAndDestroyExtentNode(struct FileExtentNode * node, struct FileExtentTree * tree)
{
    DeleteExtentNode(node, tree);
    kfree(node);
}

static void InsertExtentToExtentTree(struct FileExtentNode * newNode, struct rb_root * fileExtentRoot)
{
    struct rb_node ** new = &fileExtentRoot->rb_node;
    struct rb_node * parent = NULL;

    while (*new)
    {
        struct FileExtentNode * cur = container_of(*new, struct FileExtentNode, extentNode);
        parent = *new;
        if (newNode->fileStart < cur->fileStart)
            new = &((*new)->rb_left);
        else
            new = &((*new)->rb_right);
    }
    rb_link_node(&newNode->extentNode, parent, new);
    rb_insert_color(&newNode->extentNode, fileExtentRoot);
}

static void InsertExtentToSpaceUnmapTree(struct FileExtentNode * newNode, struct rb_root * spaceUnmapTreeRoot)
{
    struct rb_node ** new = &spaceUnmapTreeRoot->rb_node;
    struct rb_node * parent = NULL;

    while (*new)
    {
        struct FileExtentNode * cur = container_of(*new, struct FileExtentNode, spaceNode);
        parent = *new;
        if (newNode->start < cur->start)
            new = &((*new)->rb_left);
        else
            new = &((*new)->rb_right);
    }
    rb_link_node(&newNode->spaceNode, parent, new);
    rb_insert_color(&newNode->spaceNode, spaceUnmapTreeRoot);
}

static void InsertExtentNode(struct FileExtentNode * node, struct FileExtentTree * tree)
{
    struct FileSpaceNode * space;

    space = LookupFileSpaceNode(tree, node->start);
    ASSERT(space);
    InsertExtentToExtentTree(node, &tree->fileExtentRoot);
    InsertExtentToSpaceUnmapTree(node, &space->unmapRoot);
    tree->effectiveSize += node->size;
}

static void CreateAndInsertExtentNode(logic_addr_t start, UINT64 fileStart, UINT64 size, struct FileExtentTree * tree,
                                      gfp_t flags)
{
    struct FileExtentNode * node;

    node = kmalloc(sizeof(struct FileExtentNode), flags);
    node->start = start;
    node->fileStart = fileStart;
    node->size = size;
    InsertExtentNode(node, tree);
}

static void ExtentTreeErase(struct FileExtentTree * tree, UINT64 fileStart, UINT64 size, gfp_t flags)
{
    struct FileExtentNode * cur;
    struct rb_node * nextNode;
    UINT64 extentStart = fileStart;
    UINT64 extentEnd = fileStart + size;

    cur = ExtentTreeLowerBound(tree, fileStart);
    if (cur == NULL)
    {
        struct rb_node * firstNode;
        firstNode = rb_first(&tree->fileExtentRoot);
        if (firstNode == NULL)
            cur = NULL;
        else
            cur = container_of(firstNode, struct FileExtentNode, extentNode);
    }
    if (cur == NULL)
        nextNode = NULL;
    else
        nextNode = rb_next(&cur->extentNode);
    while (1)
    {
        UINT64 curStart;
        UINT64 curEnd;
        if (!cur)
            break;
        curStart = cur->fileStart;
        curEnd = cur->fileStart + cur->size;
        if (curStart >= extentEnd)
            break;
        if (curEnd <= extentStart)
            goto nextStep;
        if (curStart <= extentStart && curEnd >= extentStart && curEnd <= extentEnd)
        {
            //       |--------| extent
            // |-------|    cur
            cur->size -= (curEnd - extentStart);
            if (cur->size == 0)
                DeleteAndDestroyExtentNode(cur, tree);
            tree->effectiveSize -= (curEnd - extentStart);
        }
        else if (curStart <= extentStart && curEnd >= extentEnd)
        {
            UINT64 newExtentSize;
            //     |------|
            //  |------------|
            cur->size = extentStart - curStart;
            if (cur->size == 0)
                DeleteAndDestroyExtentNode(cur, tree);
            newExtentSize = curEnd - extentEnd;
            if (newExtentSize)
                CreateAndInsertExtentNode(cur->start + (extentEnd - curStart), extentEnd, newExtentSize, tree, flags);
            tree->effectiveSize -= (extentEnd - extentStart);
        }
        else if (curStart >= extentStart && curEnd <= extentEnd)
        {
            //  |----------|
            //    |----|
            DeleteAndDestroyExtentNode(cur, tree);
            tree->effectiveSize -= (curEnd - curStart);
        }
        else
        {
            //  |----------|
            //      |----------|
            cur->size = curEnd - extentEnd;
            if (cur->size)
            {
                cur->start += (extentEnd - curStart);
                cur->fileStart += (extentEnd - curStart);
                DeleteExtentNode(cur, tree);
                InsertExtentNode(cur, tree);
            }
            else
            {
                DeleteAndDestroyExtentNode(cur, tree);
            }
            tree->effectiveSize -= (extentEnd - curStart);
        }

    nextStep:
        if (nextNode)
            cur = container_of(nextNode, struct FileExtentNode, extentNode);
        else
            cur = NULL;
        if (nextNode)
            nextNode = rb_next(nextNode);
    }
}

int FileExtentTreeAddExtent(struct FileExtentTree * tree, logic_addr_t addr, UINT64 fileStart, UINT64 size, gfp_t flags)
{
    struct FileExtentNode * extentNode;

    extentNode = kmalloc(sizeof(struct FileExtentNode), flags);
    if (!extentNode)
        return -ENOMEM;
    extentNode->start = addr;
    extentNode->size = size;
    extentNode->fileStart = fileStart;

    ExtentTreeErase(tree, fileStart, size, flags);
    InsertExtentNode(extentNode, tree);
    return 0;
}

void FileExtentTreeRead(struct FileExtentTree * tree, UINT64 start, UINT64 end, void * buffer, struct NVMAccesser * acc)
{
    struct FileExtentNode * cur;
    struct rb_node * nextNode;
    UINT64 extentStart = start;
    UINT64 extentEnd = end;
    UINT64 lastRead = start;
    UINT64 readStart;

    cur = ExtentTreeLowerBound(tree, start);
    if (cur == NULL)
    {
        struct rb_node * firstNode;
        firstNode = rb_first(&tree->fileExtentRoot);
        if (firstNode == NULL)
            cur = NULL;
        else
            cur = container_of(firstNode, struct FileExtentNode, extentNode);
    }
    if (cur == NULL)
        nextNode = NULL;
    else
        nextNode = rb_next(&cur->extentNode);
    while (1)
    {
        logic_addr_t curCanReadStart, curCanReadEnd;
        UINT64 curStart;
        UINT64 curEnd;

        if (!cur)
            break;
        curStart = cur->fileStart;
        curEnd = cur->fileStart + cur->size;
        if (curStart >= extentEnd)
            break;
        if (curEnd <= extentStart)
            goto nextStep;
        if (curStart <= extentStart && curEnd >= extentStart && curEnd <= extentEnd)
        {
            //       |--------| extent
            // |-------|    cur
            curCanReadStart = cur->start + (extentStart - curStart);
            curCanReadEnd = cur->start + cur->size;
            readStart = extentStart;
        }
        else if (curStart <= extentStart && curEnd >= extentEnd)
        {
            //     |------|
            //  |------------|
            curCanReadStart = cur->start + (extentStart - curStart);
            curCanReadEnd = cur->start + (extentEnd - curStart);
            readStart = extentStart;
        }
        else if (curStart >= extentStart && curEnd <= extentEnd)
        {
            //  |----------|
            //    |----|
            curCanReadStart = cur->start;
            curCanReadEnd = cur->start + cur->size;
            readStart = curStart;
        }
        else
        {
            //  |----------|
            //      |----------|
            curCanReadStart = cur->start;
            curCanReadEnd = cur->start + (extentEnd - curStart);
            readStart = curStart;
        }

        if (lastRead != readStart)
        {
            memset(buffer + (readStart - start), 0, readStart - lastRead);
        }

        NVMAccesserRead(acc, curCanReadStart, curCanReadEnd - curCanReadStart, buffer + (readStart - start));
        lastRead = readStart + (curCanReadEnd - curCanReadStart);

    nextStep:
        if (nextNode)
            cur = container_of(nextNode, struct FileExtentNode, extentNode);
        else
            cur = NULL;
        if (nextNode)
            nextNode = rb_next(nextNode);
    }
    if (lastRead != end)
    {
        memset(buffer + (lastRead - start), 0, (end - lastRead));
    }
}

UINT64 FileExtentTreeGetEffectiveSize(struct FileExtentTree * tree)
{
    return tree->effectiveSize;
}

UINT64 FileExtentTreeGetSpaceSize(struct FileExtentTree * tree)
{
    return tree->spaceSize;
}

static void FileSpaceTreeDestroy(struct FileExtentTree * tree)
{
    struct rb_node *cur, *next;
    cur = rb_first(&tree->spaceRoot);
    while (cur)
    {
        next = rb_next(cur);
        rb_erase(cur, &tree->spaceRoot);
        kfree(container_of(cur, struct FileSpaceNode, node));
        cur = next;
    }
}

static void ExtentTreeDestroy(struct FileExtentTree * tree)
{
    struct rb_node *cur, *next;
    struct FileExtentNode * node;
    cur = rb_first(&tree->fileExtentRoot);
    while (cur)
    {
        next = rb_next(cur);
        rb_erase(cur, &tree->fileExtentRoot);
        node = container_of(cur, struct FileExtentNode, extentNode);
        kfree(node);
        cur = next;
    }
}

void FileExtentTreeUninit(struct FileExtentTree * tree)
{
    FileSpaceTreeDestroy(tree);
    ExtentTreeDestroy(tree);
}

UINT64 FileExtentTreeCalculateEffectiveSize(struct FileExtentTree * tree)
{
    return tree->effectiveSize;
}

static int ExtentTreeIsSame(struct FileExtentTree * tree1, struct FileExtentTree * tree2)
{
    struct rb_node *cur1, *cur2;
    cur1 = rb_first(&tree1->fileExtentRoot);
    cur2 = rb_first(&tree2->fileExtentRoot);
    while (cur1 && cur2)
    {
        struct FileExtentNode *node1, *node2;
        node1 = container_of(cur1, struct FileExtentNode, extentNode);
        node2 = container_of(cur2, struct FileExtentNode, extentNode);
        if (node1->start != node2->start)
            return 0;
        if (node1->size != node2->size)
            return 0;
        if (node1->fileStart != node2->fileStart)
            return 0;
        cur1 = rb_next(cur1);
        cur2 = rb_next(cur2);
    }
    if (cur1 || cur2)
        return 0;
    return 1;
}

static int SpaceTreeIsSame(struct FileExtentTree * tree1, struct FileExtentTree * tree2)
{
    struct rb_node *cur1, *cur2;
    cur1 = rb_first(&tree1->spaceRoot);
    cur2 = rb_first(&tree2->spaceRoot);
    while (cur1 && cur2)
    {
        struct FileSpaceNode *node1, *node2;
        node1 = container_of(cur1, struct FileSpaceNode, node);
        node2 = container_of(cur2, struct FileSpaceNode, node);
        if (node1->start != node2->start)
            return 0;
        if (node1->size != node2->size)
            return 0;
        cur1 = rb_next(cur1);
        cur2 = rb_next(cur2);
    }
    if (cur1 || cur2)
        return 0;
    return 1;
}

int FileExtentTreeIsSame(struct FileExtentTree * tree1, struct FileExtentTree * tree2)
{
    return ExtentTreeIsSame(tree1, tree2) && SpaceTreeIsSame(tree1, tree2);
}