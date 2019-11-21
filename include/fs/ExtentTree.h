#ifndef EXTENT_TREE_H
#define EXTENT_TREE_H

#include "Types.h"
#include <linux/rbtree.h>

struct Extent
{
    UINT64 start;
    UINT64 end;
};

#define DEFAULT_EXTENT_CONTAINER_NUM 3

struct ExtentContainer
{
    UINT64 size;
    UINT64 extentNum;
    UINT64 currentNum;
    struct Extent * array;
    struct Extent inlineArray[DEFAULT_EXTENT_CONTAINER_NUM];
};

struct ExtentTree
{
    struct rb_root root;
    UINT64 size;
};

void ExtentTreeInit(struct ExtentTree * tree);
void ExtentTreeUninit(struct ExtentTree * tree);
void ExtentTreePut(struct ExtentTree * tree, UINT64 start, UINT64 end, gfp_t flags);
void ExtentTreeGet(struct ExtentTree * tree, struct ExtentContainer * container, UINT64 size, gfp_t flags);
void ExtentTreeBatchPut(struct ExtentTree * tree, struct ExtentContainer * container, gfp_t flags);
void ExtentContainerInit(struct ExtentContainer * container, gfp_t flags);
void ExtentContainerUninit(struct ExtentContainer * container);
void ExtentContainerClear(struct ExtentContainer * container);
void ExtentContainerAppend(struct ExtentContainer * container, UINT64 start, UINT64 end, gfp_t flags);

#define for_each_extent_in_container(extent, container)                                                                \
    for (extent = (container)->array; extent < (container)->array + (container)->currentNum; ++extent)

#endif