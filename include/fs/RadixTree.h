#ifndef RADIX_TREE_H
#define RADIX_TREE_H

#include "Types.h"

struct RadixTreeNode
{
    struct RadixTreeNode * slots[512];
};

struct RadixTree
{
    struct RadixTreeNode * root;
};

void RadixTreeInit(struct RadixTree * root);
void RadixTreeUninit(struct RadixTree * root, void (*ValueDeleteFunc)(void *));
void * RadixTreeGet(struct RadixTree * root, UINT64 key);
void RadixTreeSetPrepare(struct RadixTree * root, UINT64 key);
void RadixTreeSet(struct RadixTree * root, UINT64 key, void * value);

#endif